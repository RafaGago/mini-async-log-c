#include <stdio.h>

#include <malc/entry_parser.h>

#include <bl/base/preprocessor.h>
#include <bl/base/time.h>
#include <bl/base/integer.h>
#include <bl/base/static_assert.h>
#include <bl/base/hex_string.h>
#include <bl/base/utility.h>
#include <bl/base/integer_printf_format.h>
/*----------------------------------------------------------------------------*/
static const char const* full_width_table[] = {
  "3",     /* malc_type_i8     */
  "3",     /* malc_type_u8     */
  "5",     /* malc_type_i16    */
  "5",     /* malc_type_u16    */
  "10",    /* malc_type_i32    */
  "10",    /* malc_type_u32    */
  nullptr, /* malc_type_float  */
  "20",    /* malc_type_i64    */
  "20",    /* malc_type_u64    */
  nullptr, /* malc_type_double */
};
/*----------------------------------------------------------------------------*/
static const char const* nibbles_width_table[] = {
  "2",  /* malc_type_i8     */
  "2",  /* malc_type_u8     */
  "4",  /* malc_type_i16    */
  "4",  /* malc_type_u16    */
  "8",  /* malc_type_i32    */
  "8",  /* malc_type_u32    */
  "8",  /* malc_type_float  */
  "16", /* malc_type_i64    */
  "16", /* malc_type_u64    */
  "16", /* malc_type_double */
};
/*----------------------------------------------------------------------------*/
static const char const* sev_strings[] = {
  MALC_EP_DEBUG,
  MALC_EP_TRACE,
  MALC_EP_NOTE,
  MALC_EP_WARN,
  MALC_EP_ERROR,
  MALC_EP_CRIT,
};
/*----------------------------------------------------------------------------*/
BL_EXPORT bl_err entry_parser_init(
  entry_parser* ep, alloc_tbl const* alloc
  )
{
  dstr_init (&ep->str, alloc);
  bl_err err = dstr_set_capacity (&ep->str, 1024);
  if (err) {
    return err;
  }
  dstr_init (&ep->fmt, alloc);
  err = dstr_set_capacity (&ep->fmt, 32);
  if (err) {
    dstr_destroy (&ep->str);
  }
  ep->rm_newlines = false;
  return err;
}
/*----------------------------------------------------------------------------*/
BL_EXPORT void entry_parser_destroy (entry_parser* ep)
{
  dstr_destroy (&ep->str);
}
/*----------------------------------------------------------------------------*/
static char const printf_int_modifs[]       = " -+0123456789WNxXo";
static char const printf_int_modifs_order[] = "aaaabbbbbbbbbccddd";
/*----------------------------------------------------------------------------*/
static bl_err append_int(
  entry_parser*       ep,
  log_argument const* arg,
  char                type,
  char const*         fmt_beg,
  char const*         fmt_end,
  char const*         printf_length,
  char                printf_type
  )
{
  dstr_append_char (&ep->fmt, '%');
  char order = 'a' - 1;
  bl_err err = bl_ok;
  /*this is just intended add functionality and avoid the most harmful things
    that a malicious user could do to printf ("%" ".*" and "."). printf should
    have its own very hardened parser, no need to do it twice." */
  while (fmt_beg < fmt_end) {
    char* v = memchr(
      printf_int_modifs, *fmt_beg, sizeof printf_int_modifs - 1
      );
    if (v) {
      char new_order = printf_int_modifs_order[v - printf_int_modifs];
      new_order = (order != 'b' || *v != '0') ? new_order : 'b';
      if (new_order < order) {
        continue;
      }
      order = new_order;
      switch (order) {
      case 'a': /* deliberate fall-through */
      case 'b':
        err = dstr_append_char (&ep->fmt, *v);
        break;
      case 'c':
        if (*v == 'W') {
          err = dstr_append(
            &ep->fmt, full_width_table[type - malc_type_i8]
            );
        }
        else {
          err = dstr_append(
            &ep->fmt, nibbles_width_table[type - malc_type_i8]
            );
        }
        break;
      case 'd':
        printf_type = *v;
      default: /* deliberate fall-through */
        goto done;
      }
      if (err) {
        return err;
      }
    }
    ++fmt_beg;
  }
done:
  err = dstr_append (&ep->fmt, printf_length);
  if (err) {
    return err;
  }
  err = dstr_append_char (&ep->fmt, printf_type);
  if (err) {
    return err;
  }
  switch (type) {
  case malc_type_i8:
  case malc_type_u8:
    return dstr_append_va (&ep->str, dstr_get (&ep->fmt), arg->vu8);
  case malc_type_i16:
  case malc_type_u16:
    return dstr_append_va (&ep->str, dstr_get (&ep->fmt), arg->vi16);
  case malc_type_i32:
  case malc_type_u32:
    return dstr_append_va (&ep->str, dstr_get (&ep->fmt), arg->vi32);
  case malc_type_i64:
  case malc_type_u64:
    return dstr_append_va (&ep->str, dstr_get (&ep->fmt), arg->vi64);
  default:
    return bl_invalid;
  }
}
/*----------------------------------------------------------------------------*/
static char const printf_float_modifs[]       = " -+0123456789.NfFeEgGaA";
static char const printf_float_modifs_order[] = "aaaabbbbbbbbbbcdddddddd";
/*----------------------------------------------------------------------------*/
static bl_err append_float(
  entry_parser*       ep,
  log_argument const* arg,
  char                type,
  char const*         fmt_beg,
  char const*         fmt_end
  )
{
  dstr_append_char (&ep->fmt, '%');
  char order       = 'a' - 1;
  bl_err err       = bl_ok;
  char printf_type = 'f';
  /*this is just intended add functionality and avoid the most harmful things
    that a malicious user could do to printf ("%" ".*" and "."). printf should
    have its own very hardened parser, no need to do it twice." */
  while (fmt_beg < fmt_end) {
    char* v = memchr(
      printf_float_modifs, *fmt_beg, sizeof printf_float_modifs - 1
      );
    if (v) {
      char new_order = printf_float_modifs_order[v - printf_float_modifs];
      new_order = (order != 'b' || *v != '0') ? new_order : 'b';
      if (new_order < order) {
        continue;
      }
      order = new_order;
      switch (order) {
      case 'a': /* deliberate fall-through */
      case 'b':
        err = dstr_append_char (&ep->fmt, *v);
        break;
      case 'c':
        err = dstr_append(
          &ep->fmt, nibbles_width_table[type - malc_type_i8]
          );
      break;
      case 'd':
        printf_type = *v;
      default: /* deliberate fall-through */
        goto done;
      }
      if (err) {
        return err;
      }
    }
    ++fmt_beg;
  }
done:
  err = dstr_append_char (&ep->fmt, printf_type);
  if (err) {
    return err;
  }
  if (type == malc_type_float) {
    return dstr_append_va (&ep->str, dstr_get (&ep->fmt), arg->vfloat);
  }
  else {
    return dstr_append_va (&ep->str, dstr_get (&ep->fmt), arg->vdouble);
  }
}
/*----------------------------------------------------------------------------*/
static bl_err append_mem (entry_parser* ep, u8 const* mem, uword size)
{
  char buff[129];
  uword runs = size / ((sizeof buff - 1) / 2);
  uword last = size % ((sizeof buff - 1) / 2);

  bl_err err = dstr_set_capacity (&ep->str, dstr_len (&ep->str) + (size * 2));
  if (unlikely (err)) {
      return err;
  }
  for (uword i = 0; i < runs; ++i) {
    bl_assert_side_effect(
      bl_bytes_to_hex_string (buff, sizeof buff, mem, (sizeof buff - 1) / 2) > 0
      );
    err = dstr_append_l (&ep->str, buff, sizeof buff - 1);
    if (unlikely (err)) {
      return err;
    }
    mem += (sizeof buff - 1) / 2;
  }
  bl_assert_side_effect(
    bl_bytes_to_hex_string (buff, sizeof buff, mem, last) >= 0
    );
  return dstr_append_l (&ep->str, buff, last * 2);
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
  dstr_clear (&ep->fmt);
  switch (type) {
  case malc_type_i8:
    return append_int (ep, arg, type, fmt_beg, fmt_end, FMT_L8, 'd');
  case malc_type_u8:
    return append_int (ep, arg, type, fmt_beg, fmt_end, FMT_L8, 'u');
  case malc_type_i16:
    return append_int (ep, arg, type, fmt_beg, fmt_end, FMT_L16, 'd');
  case malc_type_u16:
    return append_int (ep, arg, type, fmt_beg, fmt_end, FMT_L16, 'u');
  case malc_type_i32:
    return append_int (ep, arg, type, fmt_beg, fmt_end, FMT_L32, 'd');
  case malc_type_u32:
    return append_int (ep, arg, type, fmt_beg, fmt_end, FMT_L32, 'u');
  case malc_type_float:
    return append_float (ep, arg, type, fmt_beg, fmt_end);
  case malc_type_i64:
    return append_int (ep, arg, type, fmt_beg, fmt_end, FMT_L64, 'd');
  case malc_type_u64:
    return append_int (ep, arg, type, fmt_beg, fmt_end, FMT_L64, 'u');
  case malc_type_double:
    return append_float (ep, arg, type, fmt_beg, fmt_end);
  case malc_type_ptr:
    return dstr_append_va (&ep->str, "%p", arg->vptr);
  case malc_type_lit:
    return dstr_append (&ep->str, arg->vlit.lit);
  case malc_type_strcp:
    return dstr_append_l (&ep->str, arg->vstrcp.str, arg->vstrcp.len);
  case malc_type_memcp:
    return append_mem (ep, arg->vmemcp.mem, arg->vmemcp.size);
  case malc_type_strref:
    return dstr_append_l (&ep->str, arg->vstrref.str, arg->vstrref.len);
  case malc_type_memref:
    return append_mem (ep, arg->vmemref.mem, arg->vmemref.size);
  }
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
/*TODO: cfg: silent sanitize, etc.*/
static bl_err parse_text(
  entry_parser*       ep,
  char const*         fmt,
  char const*         types,
  log_argument const* args,
  uword               args_count
  )
{
  uword       arg_idx  = 0;
  char const* it       = fmt;
  char const* text_beg = fmt;
  char const* fmt_beg  = nullptr;
  //char const* value_modif = nullptr;
  bl_err err = bl_ok;
  dstr_clear (&ep->str);

  while (true) {
    /*end of entry*/
    if (*it == 0) {
      if (unlikely (fmt_beg)) {
        err = dstr_append_lit (&ep->str, MALC_EP_UNCLOSED_FMT);
        if (err) {
         return err;
        }
      }
      err = dstr_append_l (&ep->str, text_beg, it - text_beg);
      if (err) {
        return err;
      }
      break;
    }
    /*skip escaped braces*/
    if ((it[0] == '{' && it[1] == '{')) {
      if (likely (!fmt_beg)) {
        err = dstr_append_l (&ep->str, text_beg, it - text_beg + 1);
        if (err) {
          return err;
        }
        it      += 2;
        text_beg = it;
      }
      else {
        err = dstr_append_lit (&ep->str, MALC_EP_ESC_BRACES_IN_FMT);
        if (err) {
          return err;
        }
        it += 2;
      }
      continue;
    }
    /*format seq open */
    if (it[0] == '{') {
      if (likely (!fmt_beg)) {
        fmt_beg = it + 1;
        err = dstr_append_l (&ep->str, text_beg, it - text_beg);
        if (err) {
          return err;
        }
        text_beg = fmt_beg;
      }
      else {
        err = dstr_append_lit (&ep->str, MALC_EP_MISPLACED_OPEN_BRACES);
        if (err) {
          return err;
        }
      }
    }
    /*format seq close */
    else if (it[0] == '}') {
      if (likely (fmt_beg)) {
        if (likely (arg_idx < args_count)) {
          bl_assert (types[arg_idx]);
          err = append_arg (ep, &args[arg_idx], types[arg_idx], fmt_beg, it);
        }
        else {
          err = dstr_append_lit (&ep->str, MALC_EP_MISSING_ARG);
        }
        if (err) {
          return err;
        }
        text_beg = it + 1;
        fmt_beg  = nullptr;
        ++arg_idx;
      }
    }
    ++it;
  }
  if (unlikely (arg_idx < args_count)) {
    err = dstr_append_lit (&ep->str, MALC_EP_EXCESS_ARGS);
    /* print the missing arguments ??? */
  }
  return err;
}
/*----------------------------------------------------------------------------*/
BL_EXPORT bl_err entry_parser_get_log_strings(
  entry_parser* ep, log_entry const* e, log_strings* strs
  )
{
  if (unlikely(
    !e ||
    !e->entry ||
    !e->entry->info ||
    e->entry->info[0] < malc_sev_debug ||
    e->entry->info[0] > malc_sev_critical
    )) {
    bl_assert (false);
    return bl_invalid;
  }
  /* meson old versions ignored base library flags */
  static_assert_ns (sizeof e->timestamp == sizeof (u64));
  snprintf(
    ep->tstamp,
    TSTAMP_INTEGER + 2,
    "%0" pp_to_str (TSTAMP_INTEGER) FMT_U64 ".",
     e->timestamp / nsec_in_sec
    );
  snprintf(
    &ep->tstamp[TSTAMP_INTEGER + 1],
    TSTAMP_DECIMAL + 1,
    "%0" pp_to_str (TSTAMP_DECIMAL) FMT_U64,
    e->timestamp % nsec_in_sec
    );
  strs->tstamp     = ep->tstamp;
  strs->tstamp_len = sizeof ep->tstamp -1;
  bl_assert (strlen (strs->tstamp) == strs->tstamp_len);
  strs->sev     = sev_strings[e->entry->info[0] - malc_sev_debug];
  strs->sev_len = lit_len (MALC_EP_DEBUG);
  bl_err err = parse_text(
    ep, e->entry->format, &e->entry->info[1], e->args, e->args_count
    );
  strs->text     = nullptr;
  strs->text_len = 0;

  if (ep->rm_newlines) {
    err = dstr_replace_lit (&ep->str, "\n", "", 0, 0);
    if (unlikely (err)) {
      return err;
    }
    err = dstr_replace_lit (&ep->str, "\r", "", 0, 0);
    if (unlikely (err)) {
      return err;
    }
  }
  strs->text     = dstr_get (&ep->str);
  strs->text_len = dstr_len (&ep->str);
  return err;
}
/*----------------------------------------------------------------------------*/
