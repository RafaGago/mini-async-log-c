#include <string.h>

#include <bl/base/time.h>
#include <bl/base/static_integer_math.h>
#include <bl/base/preprocessor_basic.h>

#include <bl/time_extras/time_extras.h>

#include <malc/malc.h>
#include <malc/serialization.h>
#include <malc/impl/serializer.h>

#ifndef __cplusplus
  #define DECODE_NAME_BUILD(suffix) pp_tokconcat(decode, suffix)
#else
  #define DECODE_NAME_BUILD(suffix) decode
#endif
/*----------------------------------------------------------------------------*/
declare_autoarray_funcs (log_args, log_argument);
declare_autoarray_funcs (log_refs, malc_ref);
/*----------------------------------------------------------------------------*/
#if MALC_BUILTIN_COMPRESSION || (MALC_PTR_COMPRESSION && BL_WORDSIZE == 32)
static bl_err decode_compressed_32(
  compressed_header* ch, u8** mem, u8* mem_end, u32* v
  )
{
  u8 fmt     = (ch->hdr[ch->idx / 2] >> ((ch->idx & 1) * 4)) & 15;
  uword size = (fmt & 7) + 1;
  uword inv  = fmt >> 3;
  if (unlikely (*mem + size > mem_end || size > sizeof *v)) {
    return bl_mkerr (bl_invalid);
  }
  *v = 0;
  for (uword i = 0; i < size; ++i) {
    *v |= ((u32) **mem) << (i * 8);
    ++(*mem);
  }
  *v = inv ? ~*v : *v;
  ++ch->idx;
  return bl_mkok();
}
#endif
/*----------------------------------------------------------------------------*/
#if MALC_BUILTIN_COMPRESSION || (MALC_PTR_COMPRESSION && BL_WORDSIZE == 64)
static bl_err decode_compressed_64(
  compressed_header* ch, u8** mem, u8* mem_end, u64* v
  )
{
  u8 fmt     = (ch->hdr[ch->idx / 2] >> ((ch->idx & 1) * 4)) & 15;
  uword size = (fmt & 7) + 1;
  uword inv  = fmt >> 3;
  if (unlikely (*mem + size > mem_end)) {
    return bl_mkerr (bl_invalid);
  }
  *v = 0;
  for (uword i = 0; i < size; ++i) {
    *v |= ((u64) **mem) << (i * 8);
    ++(*mem);
  }
  *v = inv ? ~*v : *v;
  ++ch->idx;
  return bl_mkok();
}
#endif
/*----------------------------------------------------------------------------*/
#if MALC_COMPRESSION && BL_WORDSIZE == 64
/*----------------------------------------------------------------------------*/
static bl_err decode_compressed_ptr(
  compressed_header* ch, u8** mem, u8* mem_end, void** v
  )
{
  return decode_compressed_64 (ch, mem, mem_end, (u64*) v);
}
/*----------------------------------------------------------------------------*/
#elif MALC_COMPRESSION && BL_WORDSIZE == 32
/*----------------------------------------------------------------------------*/
static bl_err decode_compressed_ptr(
  compressed_header* ch, u8** mem, u8* mem_end, void** v
  )
{
  return decode_compressed_32 (ch, mem, mem_end, (u32*) v);
}
/*----------------------------------------------------------------------------*/
#endif
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_8) (
  compressed_header* ch, u8** mem, u8* mem_end, u8* v
  )
{
  if (unlikely (*mem + sizeof *v > mem_end)) {
    return bl_mkerr (bl_invalid);
  }
  *v = **mem;
  ++(*mem);
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_16)(
  compressed_header* ch, u8** mem, u8* mem_end, u16* v
  )
{
  if (unlikely (*mem + sizeof *v > mem_end)) {
    return bl_mkerr (bl_invalid);
  }
  memcpy (v, *mem, sizeof *v);
  *mem += sizeof *v;
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
#if MALC_BUILTIN_COMPRESSION == 0
static inline bl_err DECODE_NAME_BUILD(_32)(
  compressed_header* ch, u8** mem, u8* mem_end, u32* v
  )
{
  if (unlikely (*mem + sizeof *v > mem_end)) {
    return bl_mkerr (bl_invalid);
  }
  memcpy (v, *mem, sizeof *v);
  *mem += sizeof *v;
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_64)(
  compressed_header* ch, u8** mem, u8* mem_end, u64* v
  )
{
  if (unlikely (*mem + sizeof *v > mem_end)) {
    return bl_mkerr (bl_invalid);
  }
  memcpy (v, *mem, sizeof *v);
  *mem += sizeof *v;
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
#else /* MALC_BUILTIN_COMPRESSION == 0 */
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_32)(
  compressed_header* ch, u8** mem, u8* mem_end, u32* v
  )
{
  return decode_compressed_32 (ch, mem, mem_end, v);
}
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_64)(
  compressed_header* ch, u8** mem, u8* mem_end, u64* v
  )
{
  return decode_compressed_64 (ch, mem, mem_end, v);
}
/*----------------------------------------------------------------------------*/
#endif /* MALC_BUILTIN_COMPRESSION == 0 */
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_float)(
  compressed_header* ch, u8** mem, u8* mem_end, float* v
  )
{
  if (unlikely (*mem + sizeof *v > mem_end)) {
    return bl_mkerr (bl_invalid);
  }
  memcpy (v, *mem, sizeof *v);
  *mem += sizeof *v;
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_double)(
  compressed_header* ch, u8** mem, u8* mem_end, double* v
  )
{
  if (unlikely (*mem + sizeof *v > mem_end)) {
    return bl_mkerr (bl_invalid);
  }
  memcpy (v, *mem, sizeof *v);
  *mem += sizeof *v;
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
#if MALC_PTR_COMPRESSION == 0
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_ptr)(
  compressed_header* ch, u8** mem, u8* mem_end, void** v
  )
{
  if (unlikely (*mem + sizeof *v > mem_end)) {
    return bl_mkerr (bl_invalid);
  }
  memcpy (v, *mem, sizeof *v);
  *mem += sizeof *v;
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
#else /* MALC_PTR_COMPRESSION == 0 */
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_ptr)(
  compressed_header* ch, u8** mem, u8* mem_end, void** v
  )
{
  return decode_compressed_ptr (ch, mem, mem_end, v);
}
/*----------------------------------------------------------------------------*/
#endif /* MALC_PTR_COMPRESSION == 0 */
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_lit)(
  compressed_header* ch, u8** mem, u8* mem_end, malc_lit* v
  )
{
  return DECODE_NAME_BUILD(_ptr) (ch, mem, mem_end, (void**) &v->lit);
}
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_strcp)(
  compressed_header* ch, u8** mem, u8* mem_end, malc_strcp* v
  )
{
  bl_err err = DECODE_NAME_BUILD(_16) (ch, mem, mem_end, &v->len);
  if (unlikely (err.bl)) {
    return err;
  }
  if (unlikely (*mem + v->len > mem_end)) {
    return bl_mkerr (bl_invalid);
  }
  v->str = (char const*) *mem;
  *mem += v->len;
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_strref)(
  compressed_header* ch, u8** mem, u8* mem_end, malc_strref* v
  )
{
  bl_err err = DECODE_NAME_BUILD(_16) (ch, mem, mem_end, &v->len);
  if (unlikely (err.bl)) {
    return err;
  }
  return DECODE_NAME_BUILD(_ptr) (ch, mem, mem_end, (void**) &v->str);
}
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_memcp)(
  compressed_header* ch, u8** mem, u8* mem_end, malc_memcp* v
  )
{
  bl_err err = DECODE_NAME_BUILD(_16) (ch, mem, mem_end, &v->size);
  if (unlikely (err.bl)) {
    return err;
  }
  if (unlikely (*mem + v->size > mem_end)) {
    return bl_mkerr (bl_invalid);
  }
  v->mem = (u8 const*) *mem;
  *mem += v->size;
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_memref)(
  compressed_header* ch, u8** mem, u8* mem_end, malc_memref* v
  )
{
  bl_err err = DECODE_NAME_BUILD(_16) (ch, mem, mem_end, &v->size);
  if (unlikely (err.bl)) {
    return err;
  }
  return DECODE_NAME_BUILD(_ptr) (ch, mem, mem_end, (void**) &v->mem);
}
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_refdtor)(
  compressed_header* ch, u8** mem, u8* mem_end, malc_refdtor* v
  )
{
  bl_err err = DECODE_NAME_BUILD(_ptr) (ch, mem, mem_end, (void**) &v->func);
  if (unlikely (err.bl)) {
    return err;
  }
  return DECODE_NAME_BUILD(_ptr) (ch, mem, mem_end, (void**) &v->context);
}
/*----------------------------------------------------------------------------*/
#ifndef __cplusplus
#define decode(ch, mem, mem_end, val)\
  _Generic ((val),\
    u8*:           decode_8,\
    u16*:          decode_16,\
    u32*:          decode_32,\
    u64*:          decode_64,\
    double*:       decode_double,\
    float*:        decode_float,\
    void**:        decode_ptr,\
    malc_lit*:     decode_lit,\
    malc_strcp*:   decode_strcp,\
    malc_strref*:  decode_strref,\
    malc_memcp*:   decode_memcp,\
    malc_memref*:  decode_memref,\
    malc_refdtor*: decode_refdtor,\
    default:       wrong\
    )\
  ((ch), (mem), (mem_end), (val))
#endif
/*----------------------------------------------------------------------------*/
#if MALC_COMPRESSION == 0
/*----------------------------------------------------------------------------*/
void serializer_init(
  serializer* se, malc_const_entry const* entry, bool has_tstamp
  )
{
  se->entry      = entry;
  se->has_tstamp = has_tstamp;
  se->extra_size = sizeof se->entry + has_tstamp ? sizeof se->t : 0;
  se->ch         = nullptr;
  if (has_tstamp) {
    se->t = bl_get_ftstamp_fast();
  }
}
/*----------------------------------------------------------------------------*/
#else /* MALC_COMPRESSION == 0 */
/*----------------------------------------------------------------------------*/
void serializer_init(
  serializer* se, malc_const_entry const* entry, bool has_tstamp
  )
{
  se->entry      = entry;
  se->has_tstamp = has_tstamp;
  se->comp_entry = malc_get_compressed_ptr ((void*) entry);
  if (has_tstamp) {
    se->t = malc_get_compressed_u64 ((u64) bl_get_ftstamp_fast());
  }
  se->hdr_size   = div_ceil (has_tstamp + entry->compressed_count, 2);
  /* first "1 +" is for the formatting nibble of "comp_entry" */
  se->extra_size = 1 + ((se->comp_entry.format_nibble & 7) + 1) +
    (has_tstamp ? ((se->t.format_nibble & 7) + 1) : 0);
  se->chval.idx = 0;
  se->chval.hdr = nullptr;
  se->ch        = &se->chval;
}
/*----------------------------------------------------------------------------*/
#endif /* MALC_COMPRESSION == 0 */
/*----------------------------------------------------------------------------*/
/* write the header and return it ready to serialize write the varargs*/
malc_serializer serializer_prepare_external_serializer(
  serializer* ser, u8* node_mem, u8* mem
  )
{
  malc_serializer s;
  s.node_mem = node_mem;
#if MALC_COMPRESSION == 0
  s.field_mem = mem;
  malc_serialize (&s, (void*) ser->entry);
#else /* MALC_COMPRESSION == 0 */
  /*first byte is the entry length, wastes one nibble*/
  s.compressed_header     = mem;
  s.field_mem             = mem + 1;
  *s.compressed_header    = 0;
  s.compressed_header_idx = 0;
  malc_serialize (&s, malc_get_compressed_ptr ((void*) ser->entry));
  /*leaving space for all the size nibbles*/
  s.compressed_header_idx = 0;
  s.compressed_header     = s.field_mem;
  memset (s.compressed_header, 0, serializer_hdr_size (ser));
  s.field_mem            += serializer_hdr_size (ser);
#endif
  if (ser->has_tstamp) {
    malc_serialize (&s, ser->t);
  }
  return s;
}
/*----------------------------------------------------------------------------*/
bl_err deserializer_init (deserializer* ds, alloc_tbl const* alloc)
{
  memset (ds, 0, sizeof *ds);
#if MALC_COMPRESSION == 0
  ds->ch = nullptr;
#else
  ds->ch = &ds->chval;
#endif
  bl_err err = log_args_init (&ds->args, 16, alloc);
  if (err.bl) {
    return err;
  }
  err = log_refs_init (&ds->refs, 16, alloc);
  if (err.bl) {
    log_args_destroy (&ds->args, alloc);
  }
  return err;
}
/*----------------------------------------------------------------------------*/
void deserializer_destroy (deserializer* ds, alloc_tbl const* alloc)
{
  log_args_destroy (&ds->args, alloc);
  log_refs_destroy (&ds->refs, alloc);
}
/*----------------------------------------------------------------------------*/
void deserializer_reset (deserializer* ds)
{
  log_args_drop_tail_n (&ds->args, log_args_size (&ds->args));
  log_refs_drop_tail_n (&ds->refs, log_refs_size (&ds->refs));
  ds->entry           = nullptr;
  ds->refdtor.func    = nullptr;
  ds->refdtor.context = nullptr;
#if MALC_COMPRESSION
  ds->ch->idx = 0;
  ds->ch->hdr = nullptr;
#endif
}
/*----------------------------------------------------------------------------*/
bl_err deserializer_execute(
  deserializer*    ds,
  u8*              mem,
  u8*              mem_end,
  bool             has_timestamp,
  alloc_tbl const* alloc
  )
{
#if MALC_COMPRESSION == 0
  void* entry;
  bl_err err = decode (ds->ch, &mem, mem_end, &entry);
  if (unlikely (err.bl)) {
    return err;
  }
  ds->entry = (malc_const_entry const*) entry;
#else
  void* entry;
  ds->entry   = nullptr;
  ds->ch->hdr = mem;
  ++mem;
  bl_err err = decode_compressed_ptr(ds->ch, &mem, mem_end, &entry);
  if (unlikely (err.bl)) {
    return err;
  }
  ds->entry   = (malc_const_entry const*) entry;
  ds->ch->hdr = mem;
  ds->ch->idx = 0;
  mem        += div_ceil (ds->entry->compressed_count + has_timestamp, 2);
#endif
  if (has_timestamp) {
    ds->t = 0;
    err   = decode (ds->ch, &mem, mem_end, &ds->t);
    if (unlikely (err.bl)) {
      return err;
    }
  }
  else {
    ds->t = bl_get_ftstamp_fast();
  }
  ds->t = bl_fstamp_to_nsec (ds->t);
  char const* partype = &ds->entry->info[1];
  log_argument larg;

  while (*partype) {
    uword push_this_arg = true;
    switch (*partype) {
    case malc_type_i8:
    case malc_type_u8:
      err = decode (ds->ch, &mem, mem_end, &larg.vu8);
      break;
    case malc_type_i16:
    case malc_type_u16:
      err = decode (ds->ch, &mem, mem_end, &larg.vu16);
      break;
    case malc_type_i32:
    case malc_type_u32:
      err = decode (ds->ch, &mem, mem_end, &larg.vu32);
      break;
    case malc_type_float:
      err = decode (ds->ch, &mem, mem_end, &larg.vfloat);
      break;
    case malc_type_i64:
    case malc_type_u64:
      err = decode (ds->ch, &mem, mem_end, &larg.vu64);
      break;
    case malc_type_double:
      err = decode (ds->ch, &mem, mem_end, &larg.vdouble);
      break;
    case malc_type_ptr:
      err = decode (ds->ch, &mem, mem_end, &larg.vptr);
      break;
    case malc_type_lit:
      err = decode (ds->ch, &mem, mem_end, &larg.vlit);
      break;
    case malc_type_strcp:
    case malc_type_memcp:
      err = decode (ds->ch, &mem, mem_end, &larg.vstrcp);
      break;
    case malc_type_strref:
    case malc_type_memref:
      err = decode (ds->ch, &mem, mem_end, &larg.vstrref);
      if (likely (!err.bl)) {
        malc_ref r;
        r.ref  = larg.vstrref.str;
        r.size = larg.vstrref.len;
        err = log_refs_insert_tail (&ds->refs, &r, alloc);
      }
      break;
    case malc_type_refdtor:
      err = decode (ds->ch, &mem, mem_end, &ds->refdtor);
      push_this_arg = false;
      break;
    default:
      bl_assert (0 && "bug");
      return bl_mkerr (bl_invalid);
      break;
    } /* switch */
    if (!err.bl && push_this_arg) {
      err = log_args_insert_tail (&ds->args, &larg, alloc);
    }
    if (unlikely (err.bl)) {
      return err;
    }
    ++partype;
  }
  return err;
}
/*----------------------------------------------------------------------------*/
log_entry deserializer_get_log_entry (deserializer const* ds)
{
  log_entry le;
  le.entry      = ds->entry;
  le.timestamp  = ds->t;
  le.refdtor    = ds->refdtor;
  le.args       = log_args_beg (&ds->args);
  le.args_count = log_args_size (&ds->args);
  le.refs       = log_refs_beg (&ds->refs);
  le.refs_count = log_refs_size (&ds->refs);
  return le;
}
/*----------------------------------------------------------------------------*/
