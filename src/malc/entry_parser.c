#include <stdio.h>

#include <malc/entry_parser.h>

#include <bl/base/preprocessor.h>
#include <bl/base/integer_short.h>
#include <bl/base/static_assert.h>
#include <bl/base/hex_string.h>
#include <bl/base/utility.h>
#define BL_UNPREFIXED_PRINTF_FORMATS
#include <bl/base/time.h>
#include <bl/base/integer_printf_format.h>

#include <bl/tostr/itostr.h>
#define MALC_EP_SEPARATOR " "
/*----------------------------------------------------------------------------*/
static const char* const sev_strings[] = {
  MALC_EP_DEBUG,
  MALC_EP_TRACE,
  MALC_EP_NOTE,
  MALC_EP_WARN,
  MALC_EP_ERROR,
  MALC_EP_CRIT,
};
/*----------------------------------------------------------------------------*/
bl_err entry_parser_init (entry_parser* ep, bl_alloc_tbl const* alloc)
{
  ep->alloc = alloc;
  bl_dstr_init (&ep->str, alloc);
  bl_err err = bl_dstr_set_capacity (&ep->str, 1024);
  if (err.own) {
    return err;
  }
  bl_dstr_init (&ep->fmt, alloc);
  err = bl_dstr_set_capacity (&ep->fmt, 32);
  if (err.own) {
    bl_dstr_destroy (&ep->str);
  }
  ep->sanitize_log_entries = false;
  return err;
}
/*----------------------------------------------------------------------------*/
void entry_parser_destroy (entry_parser* ep)
{
  bl_dstr_destroy (&ep->fmt);
  bl_dstr_destroy (&ep->str);
}
/*----------------------------------------------------------------------------*/
static bl_err append_int(
  entry_parser*       ep,
  log_argument const* arg,
  char                type,
  char const*         fmt_beg,
  char const*         fmt_end
  )
{
  bl_dstr_append_l (&ep->fmt, fmt_beg, fmt_end - fmt_beg);
  bl_dstrbuf str = bl_dstr_steal_ownership (&ep->str);
  bl_err err;

  switch (type) {
  case malc_type_i8:
    err = bl_itostr_dyn_i (&str, bl_dstr_get (&ep->fmt), arg->vi8);
    break;
  case malc_type_u8:
    err = bl_itostr_dyn_u (&str, bl_dstr_get (&ep->fmt), arg->vu8);
    break;
  case malc_type_i16:
    err = bl_itostr_dyn_i (&str, bl_dstr_get (&ep->fmt), arg->vi16);
    break;
  case malc_type_u16:
    err = bl_itostr_dyn_u (&str, bl_dstr_get (&ep->fmt), arg->vu16);
    break;
  case malc_type_i32:
    err = bl_itostr_dyn_i (&str, bl_dstr_get (&ep->fmt), arg->vi32);
    break;
  case malc_type_u32:
    err = bl_itostr_dyn_u (&str, bl_dstr_get (&ep->fmt), arg->vu32);
    break;
  case malc_type_i64:
    err = bl_itostr_dyn_i (&str, bl_dstr_get (&ep->fmt), arg->vi64);
    break;
  case malc_type_u64:
    err = bl_itostr_dyn_u (&str, bl_dstr_get (&ep->fmt), arg->vu64);
    break;
  default:
    err = bl_mkerr (bl_invalid);
    break;
  }
  bl_dstr_transfer_ownership (&ep->str, &str);
  return err;
}
/*----------------------------------------------------------------------------*/
static bl_err append_int_array(
  entry_parser* ep,
  char const*   fmt_beg,
  char const*   fmt_end,
  char const*   sep,
  void const*   v,
  size_t        v_count,
  bool          is_signed,
  bl_uword      sz_log2
  )
{
  bl_dstr_append_l (&ep->fmt, fmt_beg, fmt_end - fmt_beg);
  bl_dstrbuf str = bl_dstr_steal_ownership (&ep->str);
  bl_err err = bl_itostr_dyn_arr(
    &str, bl_dstr_get (&ep->fmt), sep, v, v_count, is_signed, sz_log2
    );
  bl_dstr_transfer_ownership (&ep->str, &str);
  return err;
}
/*----------------------------------------------------------------------------*/
static char const printf_float_modifs[]       = "# -+0123456789.fFeEgGaA";
static char const printf_float_modifs_order[] = "aaaaabbbbbbbbbbcccccccc";
/*----------------------------------------------------------------------------*/
static bl_err build_float_format_on_ep_fmt(
  entry_parser* ep, char const* fmt_beg, char const* fmt_end
  )
{
  /*fills ep->fmt*/
  bl_dstr_append_char (&ep->fmt, '%');
  char order        = 'a' - 1;
  bl_err err        = bl_mkok();
  char printf_type  = 'f';
  uword dotcount    = 0;
  /*this is just intended as a quick filter for the most egregious things that
  could be passed to printf, e.g. added "%", ".*", etc. and to add the extra
  length modifiers. A good printf implementation on any major platform should
  have its own battle-tested parser, no need to do it twice." */
  while (fmt_beg < fmt_end) {
    char* v = (char*) memchr(
      printf_float_modifs, *fmt_beg, sizeof printf_float_modifs - 1
      );
    if (v) {
      char new_order = printf_float_modifs_order[v - printf_float_modifs];
      /* '0' is allowed as a flag and as a width/precision number. manually
      correcting the new_order */
      new_order = (order != 'b' || *v != '0') ? new_order : 'b';
      if (new_order < order) {
        continue;
      }
      order = new_order;
      switch (order) {
      case 'a': /* deliberate fall-through */
      case 'b':
        if (*v == '.') {
          ++dotcount;
          if (dotcount > 1) {
            continue; /* only one dot allowed */
          }
        }
        err = bl_dstr_append_char (&ep->fmt, *v);
        break;
      case 'c':
        printf_type = *v;
      default: /* deliberate fall-through */
        goto done;
      }
      if (err.own) {
        return err;
      }
    }
    ++fmt_beg;
  }
done:
  err = bl_dstr_append_char (&ep->fmt, printf_type);
  return err;
}
/*----------------------------------------------------------------------------*/
static bl_err append_float(
  entry_parser*       ep,
  log_argument const* arg,
  char                type,
  char const*         fmt_beg,
  char const*         fmt_end
  )
{
  bl_err err = build_float_format_on_ep_fmt (ep, fmt_beg, fmt_end);
  if (bl_unlikely (err.own)) {
    return err;
  }
  if (type == malc_type_float) {
    return bl_dstr_append_va (&ep->str, 1, bl_dstr_get (&ep->fmt), arg->vfloat);
  }
  else {
    return bl_dstr_append_va(
      &ep->str, 1, bl_dstr_get (&ep->fmt), arg->vdouble
      );
  }
}
/*----------------------------------------------------------------------------*/
static bl_err append_mem (entry_parser* ep, u8 const* mem, uword size)
{
  char buff[129];
  uword runs = size / ((sizeof buff - 1) / 2);
  uword last = size % ((sizeof buff - 1) / 2);

  bl_err err = bl_dstr_set_capacity(
    &ep->str, bl_dstr_len (&ep->str) + (size * 2)
    );
  if (bl_unlikely (err.own)) {
      return err;
  }
  for (uword i = 0; i < runs; ++i) {
    bl_assert_side_effect(
      bl_bytes_to_hex_string (buff, sizeof buff, mem, (sizeof buff - 1) / 2) > 0
      );
    err = bl_dstr_append_l (&ep->str, buff, sizeof buff - 1);
    if (bl_unlikely (err.own)) {
      return err;
    }
    mem += (sizeof buff - 1) / 2;
  }
  bl_assert_side_effect(
    bl_bytes_to_hex_string (buff, sizeof buff, mem, last) >= 0
    );
  return bl_dstr_append_l (&ep->str, buff, last * 2);
}
/*----------------------------------------------------------------------------*/
static inline malc_obj_ref get_aligned_obj_ref(
  entry_parser* ep, malc_obj const* obj
  )
{
  malc_obj_ref r;
  memcpy (&ep->objstorage, obj->obj, obj->table->obj_sizeof);
  r.obj = (void*) &ep->objstorage;
  r.extra.context = nullptr;
  return r;
}
/*----------------------------------------------------------------------------*/
static inline malc_obj_ref get_aligned_obj_ref_flag(
  entry_parser* ep, malc_obj_flag const* obj
  )
{
  malc_obj_ref r = get_aligned_obj_ref (ep, &obj->base);
  r.extra.flag = obj->flag;
  return r;
}
/*----------------------------------------------------------------------------*/
static inline malc_obj_ref get_aligned_obj_ref_ctx(
  entry_parser* ep, malc_obj_ctx const* obj
  )
{
  malc_obj_ref r = get_aligned_obj_ref (ep, &obj->base);
  r.extra.context = obj->context;
  return r;
}
/*----------------------------------------------------------------------------*/
static bl_err append_arg(
  entry_parser*       ep,
  log_argument const* arg,
  char                type,
  char const*         fmt_beg,
  char const*         fmt_end
  );
/*----------------------------------------------------------------------------*/
static int push_obj_data(
  void*                    epv,
  char const*              fmt_beg,
  char const*              fmt_end,
  malc_obj_log_data const* ld
  )
{
  entry_parser* ep = (entry_parser*) epv;
  bl_err err;

  if (bl_unlikely (!ep || !fmt_beg || !fmt_end || !ld)) {
    bl_assert (false && "bug");
    return bl_invalid;
  }

  bl_dstr_clear (&ep->fmt);
  if (ld->is_str) {
    err = bl_dstr_append_l (&ep->str, ld->data.str.ptr, ld->data.str.len);
    return (int) err.own;
  }

  if (bl_unlikely (ld->data.builtin.type > malc_obj_double)) {
    bl_assert (false && "bug");
    return bl_invalid;
  }

  if (ld->data.builtin.count == 0 || ld->data.builtin.ptr == nullptr) {
    //empty
    return 0;
  }

  if (ld->data.builtin.type <= malc_obj_i64) {
    return (int) append_int_array(
      ep,
      fmt_beg,
      fmt_end,
      MALC_EP_SEPARATOR,
      ld->data.builtin.ptr,
      ld->data.builtin.count,
      ld->data.builtin.type >= malc_obj_i8,
      ld->data.builtin.type & 3 /* mask on last two bits */
      ).own;
  }

  /* using printf for floats */
  uword prev_len = bl_dstr_len (&ep->str);
  err = build_float_format_on_ep_fmt (ep, fmt_beg, fmt_end);

  for (uword i = 0; (i < ld->data.builtin.count) && !err.own; ++i) {
    uword len = bl_dstr_len (&ep->str);
    if (prev_len < len) {
      /* separator between values */
      err = bl_dstr_append_lit (&ep->str, MALC_EP_SEPARATOR);
      if (err.own) {
        break;
      }
      prev_len = len;
    }
    if (ld->data.builtin.type == malc_type_float) {
      err = bl_dstr_append_va(
        &ep->str,
        10,
        bl_dstr_get (&ep->fmt),
        ((float const*) ld->data.builtin.ptr)[i]
        );
    }
    else {
      err = bl_dstr_append_va(
        &ep->str,
        10,
        bl_dstr_get (&ep->fmt),
        ((double const*) ld->data.builtin.ptr)[i]
        );
    }
  }
  return (int) err.own;
}
/*----------------------------------------------------------------------------*/
static bl_err append_obj(
  entry_parser*   ep,
  char const*     fmt_beg,
  char const*     fmt_end,
  malc_obj const* obj,
  malc_obj_ref    od
  )
{
  if (bl_unlikely (!obj->table->getdata)) {
    return bl_mkerr (bl_invalid);
  }
  malc_obj_push_context pc;
  pc.push        = push_obj_data;
  pc.pushcontext = (void*) ep;
  pc.fmt_beg     = fmt_beg;
  pc.fmt_end     = fmt_end;
  int err = obj->table->getdata (&od, obj->table, &pc, ep->alloc);
  return bl_mkerr (!err ? bl_ok: bl_error);
}
/*----------------------------------------------------------------------------*/
static bl_err append_arg(
  entry_parser*       ep,
  log_argument const* arg,
  char                type,
  char const*         fmt_beg,
  char const*         fmt_end
  )
{
  bl_dstr_clear (&ep->fmt);
  switch (type) {
  case malc_type_i8:
    return append_int (ep, arg, type, fmt_beg, fmt_end);
  case malc_type_u8:
    return append_int (ep, arg, type, fmt_beg, fmt_end);
  case malc_type_i16:
    return append_int (ep, arg, type, fmt_beg, fmt_end);
  case malc_type_u16:
    return append_int (ep, arg, type, fmt_beg, fmt_end);
  case malc_type_i32:
    return append_int (ep, arg, type, fmt_beg, fmt_end);
  case malc_type_u32:
    return append_int (ep, arg, type, fmt_beg, fmt_end);
  case malc_type_float:
    return append_float (ep, arg, type, fmt_beg, fmt_end);
  case malc_type_i64:
    return append_int (ep, arg, type, fmt_beg, fmt_end);
  case malc_type_u64:
    return append_int (ep, arg, type, fmt_beg, fmt_end);
  case malc_type_double:
    return append_float (ep, arg, type, fmt_beg, fmt_end);
  case malc_type_ptr:
    return bl_dstr_append_va (&ep->str, 1, "%p", arg->vptr);
  case malc_type_lit:
    return bl_dstr_append (&ep->str, arg->vlit.lit);
  case malc_type_strcp:
    return bl_dstr_append_l (&ep->str, arg->vstrcp.str, arg->vstrcp.len);
  case malc_type_memcp:
    return append_mem (ep, arg->vmemcp.mem, arg->vmemcp.size);
  case malc_type_strref:
    return bl_dstr_append_l (&ep->str, arg->vstrref.str, arg->vstrref.len);
  case malc_type_memref:
    return append_mem (ep, arg->vmemref.mem, arg->vmemref.size);
  case malc_type_obj:
    return append_obj(
      ep, fmt_beg, fmt_end, &arg->vobj, get_aligned_obj_ref (ep, &arg->vobj)
      );
  case malc_type_obj_ctx:
    return append_obj(
      ep,
      fmt_beg,
      fmt_end,
      &arg->vobjctx.base,
      get_aligned_obj_ref_ctx (ep, &arg->vobjctx)
      );
  case malc_type_obj_flag:
    return append_obj(
      ep,
      fmt_beg,
      fmt_end,
      &arg->vobjflag.base,
      get_aligned_obj_ref_flag (ep, &arg->vobjflag)
      );
  default:
    return bl_mkok();
  }
}
/*----------------------------------------------------------------------------*/
/*TODO: cfg: silent sanitize, etc.*/
static bl_err parse_text(
  entry_parser*       ep,
  char const*         fmt,
  char const*         types,
  log_argument const* args,
  uword            args_count
  )
{
  uword       arg_idx  = 0;
  char const* it       = fmt;
  char const* text_beg = fmt;
  char const* fmt_beg  = nullptr;
  //char const* value_modif = nullptr;
  bl_err err = bl_mkok();
  bl_dstr_clear (&ep->str);

  while (true) {
    /*end of entry*/
    if (*it == 0) {
      if (bl_unlikely (fmt_beg)) {
        err = bl_dstr_append_lit (&ep->str, MALC_EP_UNCLOSED_FMT);
        if (err.own) {
         return err;
        }
      }
      err = bl_dstr_append_l (&ep->str, text_beg, it - text_beg);
      if (err.own) {
        return err;
      }
      break;
    }
    /*skip escaped braces*/
    if ((it[0] == '{' && it[1] == '{')) {
      if (bl_likely (!fmt_beg)) {
        err = bl_dstr_append_l (&ep->str, text_beg, it - text_beg + 1);
        if (err.own) {
          return err;
        }
        it      += 2;
        text_beg = it;
      }
      else {
        err = bl_dstr_append_lit (&ep->str, MALC_EP_ESC_BRACES_IN_FMT);
        if (err.own) {
          return err;
        }
        it += 2;
      }
      continue;
    }
    /*format seq open */
    if (it[0] == '{') {
      if (bl_likely (!fmt_beg)) {
        fmt_beg = it + 1;
        err = bl_dstr_append_l (&ep->str, text_beg, it - text_beg);
        if (err.own) {
          return err;
        }
        text_beg = fmt_beg;
      }
      else {
        err = bl_dstr_append_lit (&ep->str, MALC_EP_MISPLACED_OPEN_BRACES);
        if (err.own) {
          return err;
        }
      }
    }
    /*format seq close */
    else if (it[0] == '}') {
      if (bl_likely (fmt_beg)) {
        if (bl_likely (arg_idx < args_count)) {
          bl_assert (types[arg_idx]);
          err = append_arg (ep, &args[arg_idx], types[arg_idx], fmt_beg, it);
        }
        else {
          err = bl_dstr_append_lit (&ep->str, MALC_EP_MISSING_ARG);
        }
        if (err.own) {
          return err;
        }
        text_beg = it + 1;
        fmt_beg  = nullptr;
        ++arg_idx;
      }
    }
    ++it;
  }
  if (bl_unlikely (arg_idx < args_count)) {
    err = bl_dstr_append_lit (&ep->str, MALC_EP_EXCESS_ARGS);
    /* print the missing arguments ??? */
  }
  return err;
}
/*----------------------------------------------------------------------------*/
static inline void destroy_obj (malc_obj const* obj, malc_obj_ref od)
{
  if (obj->table->destroy) {
    return obj->table->destroy (&od, obj->table);
  }
}
/*----------------------------------------------------------------------------*/
static inline void destroy_objects(
  entry_parser*       ep,
  char const*         types,
  log_argument const* args,
  uword            args_count
  )
{
  for (uword i = 0; i < args_count; ++i) {
    log_argument const* arg = &args[i];
    switch (types[i]) {
    case malc_type_obj:
      destroy_obj (&arg->vobj, get_aligned_obj_ref (ep, &arg->vobj));
      break;
    case malc_type_obj_ctx:
      destroy_obj(
        &arg->vobjctx.base, get_aligned_obj_ref_ctx (ep, &arg->vobjctx)
        );
      break;
    case malc_type_obj_flag:
      destroy_obj(
        &arg->vobjflag.base, get_aligned_obj_ref_flag (ep, &arg->vobjflag)
        );
      break;
    default:
      break;
    }
  }
}
/*----------------------------------------------------------------------------*/
bl_err entry_parser_get_log_strings(
  entry_parser* ep, log_entry const* e, malc_log_strings* strs
  )
{
  if (bl_unlikely(
    !e ||
    !e->entry ||
    !e->entry->info ||
    e->entry->info[0] < malc_sev_debug ||
    e->entry->info[0] > malc_sev_critical
    )) {
    bl_assert (false && "bug or corruption");
    return bl_mkerr (bl_invalid);
  }
  /* meson old versions ignored base library flags */
  bl_static_assert_ns_funcscope (sizeof e->timestamp == sizeof (u64));
  snprintf(
    ep->timestamp,
    TSTAMP_INTEGER + 2,
    "%0" bl_pp_to_str (TSTAMP_INTEGER) FMT_U64 ".",
     e->timestamp / bl_nsec_in_sec
    );
  snprintf(
    &ep->timestamp[TSTAMP_INTEGER + 1],
    TSTAMP_DECIMAL + 1,
    "%0" bl_pp_to_str (TSTAMP_DECIMAL) FMT_U64,
    e->timestamp % bl_nsec_in_sec
    );
  strs->timestamp     = ep->timestamp;
  strs->timestamp_len = sizeof ep->timestamp -1;
  bl_assert (strlen (strs->timestamp) == strs->timestamp_len);
  strs->sev     = sev_strings[e->entry->info[0] - malc_sev_debug];
  strs->sev_len = bl_lit_len (MALC_EP_DEBUG);
  bl_err err    = parse_text(
    ep, e->entry->format, &e->entry->info[1], e->args, e->args_count
    );
  strs->text     = nullptr;
  strs->text_len = 0;

  if (ep->sanitize_log_entries) {
    err = bl_dstr_replace_lit (&ep->str, "\n", "", 0, 0);
    if (bl_unlikely (err.own)) {
      goto free_entry_resources;
    }
    err = bl_dstr_replace_lit (&ep->str, "\r", "", 0, 0);
    if (bl_unlikely (err.own)) {
      goto free_entry_resources;
    }
  }
  strs->text     = bl_dstr_get (&ep->str);
  strs->text_len = bl_dstr_len (&ep->str);
free_entry_resources:
  destroy_objects (ep, &e->entry->info[1], e->args, e->args_count);
  if (e->refdtor.func) {
    e->refdtor.func (e->refdtor.context, e->refs, e->refs_count);
  }
  return err;
}
/*----------------------------------------------------------------------------*/
