#include <string.h>

#include <bl/base/time.h>
#include <bl/base/static_integer_math.h>
#include <bl/base/preprocessor_basic.h>

#include <malc/malc.h>
#include <malc/stack_args.h>
#include <malc/serialization.h>

#ifndef __cplusplus
  #define ENCODE_NAME_BUILD(suffix) pp_tokconcat(encode, suffix)
  #define DECODE_NAME_BUILD(suffix) pp_tokconcat(decode, suffix)
#else
  #define ENCODE_NAME_BUILD(suffix) encode
  #define DECODE_NAME_BUILD(suffix) decode
#endif
/*----------------------------------------------------------------------------*/
static inline void wrong (void) {}
/*----------------------------------------------------------------------------*/
static inline u8* ENCODE_NAME_BUILD(_8) (compressed_header* ch, u8* mem, u8 v)
{
  *mem = v;
  return mem + 1;
}
/*----------------------------------------------------------------------------*/
static inline u8* ENCODE_NAME_BUILD(_16) (compressed_header* ch, u8* mem, u16 v)
{
  memcpy (mem, &v, sizeof v);
  return mem + sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline u8* ENCODE_NAME_BUILD(_32) (compressed_header* ch, u8* mem, u32 v)
{
  memcpy (mem, &v, sizeof v);
  return mem + sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline u8* ENCODE_NAME_BUILD(_64) (compressed_header* ch, u8* mem, u64 v)
{
  memcpy (mem, &v, sizeof v);
  return mem + sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline u8* ENCODE_NAME_BUILD(_float)(
  compressed_header* ch, u8* mem, float v
  )
{
  memcpy (mem, &v, sizeof v);
  return mem + sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline u8* ENCODE_NAME_BUILD(_double)(
  compressed_header* ch, u8* mem, double v
  )
{
  memcpy (mem, &v, sizeof v);
  return mem + sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline u8* ENCODE_NAME_BUILD(_comp32)(
  compressed_header* ch, u8* mem, malc_compressed_32 v
  )
{
  uword size = (v.format_nibble & ((1 << 3) - 1)) + 1;
  bl_assert (size <= sizeof (u32));
  ch->hdr[ch->idx / 2] |= (u8) v.format_nibble << ((ch->idx & 1) * 4);
  ++ch->idx;
  for (uword i = 0; i < size; ++i) {
    *mem = (u8) (v.v >> (i * 8));
    ++mem;
  }
  return mem;
}
/*----------------------------------------------------------------------------*/
static inline u8* ENCODE_NAME_BUILD(_comp64)(
  compressed_header* ch, u8* mem, malc_compressed_64 v
  )
{
  uword size = (v.format_nibble & ((1 << 3) - 1)) + 1;
  ch->hdr[ch->idx / 2] |= (u8) v.format_nibble << ((ch->idx & 1) * 4);
  ++ch->idx;
  for (uword i = 0; i < size; ++i) {
    *mem = (u8) (v.v >> (i * 8));
    ++mem;
  }
  return mem;
}
/*----------------------------------------------------------------------------*/
static inline u8* ENCODE_NAME_BUILD(_ptr)(
  compressed_header* ch, u8* mem, void* v
  )
{
  memcpy (mem, &v, sizeof v);
  return mem + sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline u8* ENCODE_NAME_BUILD(_lit)(
  compressed_header* ch, u8* mem, malc_lit v
  )
{
  return ENCODE_NAME_BUILD(_ptr) (ch, mem, (void*) v.lit);
}
/*----------------------------------------------------------------------------*/
static inline u8* ENCODE_NAME_BUILD(_strcp)(
  compressed_header* ch, u8* mem, malc_strcp v
  )
{
  mem = ENCODE_NAME_BUILD(_16) (ch, mem, v.len);
  memcpy (mem, v.str, v.len);
  return mem + v.len;
}
/*----------------------------------------------------------------------------*/
static inline u8* ENCODE_NAME_BUILD(_memcp)(
  compressed_header* ch, u8* mem, malc_memcp v
  )
{
  mem = ENCODE_NAME_BUILD(_16) (ch, mem, v.size);
  memcpy (mem, v.mem, v.size);
  return mem + v.size;
}
/*----------------------------------------------------------------------------*/
#ifndef __cplusplus
#define encode(ch, mem, val)\
  _Generic ((val),\
    u8:                 encode_8,\
    u16:                encode_16,\
    u32:                encode_32,\
    u64:                encode_64,\
    double:             encode_double,\
    float:              encode_float,\
    void*:              encode_ptr,\
    malc_compressed_32: encode_comp32,\
    malc_compressed_64: encode_comp64,\
    malc_lit:           encode_lit,\
    malc_strcp:         encode_strcp,\
    malc_memcp:         encode_memcp,\
    default:            wrong\
    )\
  ((ch), (mem), (val))
#endif
/*----------------------------------------------------------------------------*/
#if !defined (MALC_NO_BUILTIN_COMPRESSION) || \
    !defined (MALC_NO_PTR_COMPRESSION) && BL_WORDSIZE == 32
static bl_err decode_compressed_32(
  compressed_header* ch, u8** mem, u8* mem_end, u32* v
  )
{
  u8 fmt     = (ch->hdr[ch->idx / 2] >> ((ch->idx & 1) * 4)) & 15;
  uword size = (fmt & 7) + 1;
  uword inv  = fmt >> 3;
  if (unlikely (*mem + size > mem_end || size > sizeof *v)) {
    return bl_invalid;
  }
  *v = 0;
  for (uword i = 0; i < size; ++i) {
    *v |= ((u32) **mem) << (i * 8);
    ++(*mem);
  }
  *v = inv ? ~*v : *v;
  ++ch->idx;
  return bl_ok;
}
#endif
/*----------------------------------------------------------------------------*/
#if !defined (MALC_NO_BUILTIN_COMPRESSION) || \
    !defined (MALC_NO_PTR_COMPRESSION) && BL_WORDSIZE == 64
static bl_err decode_compressed_64(
  compressed_header* ch, u8** mem, u8* mem_end, u64* v
  )
{
  u8 fmt     = (ch->hdr[ch->idx / 2] >> ((ch->idx & 1) * 4)) & 15;
  uword size = (fmt & 7) + 1;
  uword inv  = fmt >> 3;
  if (unlikely (*mem + size > mem_end)) {
    return bl_invalid;
  }
  *v = 0;
  for (uword i = 0; i < size; ++i) {
    *v |= ((u64) **mem) << (i * 8);
    ++(*mem);
  }
  *v = inv ? ~*v : *v;
  ++ch->idx;
  return bl_ok;
}
#endif
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_8) (
  compressed_header* ch, u8** mem, u8* mem_end, u8* v
  )
{
  if (unlikely (*mem + sizeof *v > mem_end)) {
    return bl_invalid;
  }
  *v = **mem;
  ++(*mem);
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_16)(
  compressed_header* ch, u8** mem, u8* mem_end, u16* v
  )
{
  if (unlikely (*mem + sizeof *v > mem_end)) {
    return bl_invalid;
  }
  memcpy (v, *mem, sizeof *v);
  *mem += sizeof *v;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
#ifdef MALC_NO_BUILTIN_COMPRESSION
static inline bl_err DECODE_NAME_BUILD(_32)(
  compressed_header* ch, u8** mem, u8* mem_end, u32* v
  )
{
  if (unlikely (*mem + sizeof *v > mem_end)) {
    return bl_invalid;
  }
  memcpy (v, *mem, sizeof *v);
  *mem += sizeof *v;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_64)(
  compressed_header* ch, u8** mem, u8* mem_end, u64* v
  )
{
  if (unlikely (*mem + sizeof *v > mem_end)) {
    return bl_invalid;
  }
  memcpy (v, *mem, sizeof *v);
  *mem += sizeof *v;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
#else /* MALC_NO_BUILTIN_COMPRESSION */
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
#endif /* MALC_NO_BUILTIN_COMPRESSION */
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_float)(
  compressed_header* ch, u8** mem, u8* mem_end, float* v
  )
{
  if (unlikely (*mem + sizeof *v > mem_end)) {
    return bl_invalid;
  }
  memcpy (v, *mem, sizeof *v);
  *mem += sizeof *v;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_double)(
  compressed_header* ch, u8** mem, u8* mem_end, double* v
  )
{
  if (unlikely (*mem + sizeof *v > mem_end)) {
    return bl_invalid;
  }
  memcpy (v, *mem, sizeof *v);
  *mem += sizeof *v;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
#ifdef MALC_NO_PTR_COMPRESSION
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_ptr)(
  compressed_header* ch, u8** mem, u8* mem_end, void** v
  )
{
  if (unlikely (*mem + sizeof *v > mem_end)) {
    return bl_invalid;
  }
  memcpy (v, *mem, sizeof *v);
  *mem += sizeof *v;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
#elif BL_WORDSIZE == 64
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_ptr)(
  compressed_header* ch, u8** mem, u8* mem_end, void** v
  )
{
  return decode_compressed_64 (ch, mem, mem_end, (u64*) v);
}
/*----------------------------------------------------------------------------*/
#else
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_ptr)(
  compressed_header* ch, u8** mem, u8* mem_end, void** v
  )
{
  return decode_compressed_32 (ch, mem, mem_end, (u32*) v);
}
/*----------------------------------------------------------------------------*/
#endif
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
  if (unlikely (err)) {
    return err;
  }
  if (unlikely (*mem + v->len > mem_end)) {
    return bl_invalid;
  }
  v->str = (char const*) *mem;
  *mem += v->len;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_memcp)(
  compressed_header* ch, u8** mem, u8* mem_end, malc_memcp* v
  )
{
  bl_err err = DECODE_NAME_BUILD(_16) (ch, mem, mem_end, &v->size);
  if (unlikely (err)) {
    return err;
  }
  if (unlikely (*mem + v->size > mem_end)) {
    return bl_invalid;
  }
  v->mem = (u8 const*) *mem;
  *mem += v->size;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
#ifndef __cplusplus
#define decode(ch, mem, mem_end, val)\
  _Generic ((val),\
    u8*:         decode_8,\
    u16*:        decode_16,\
    u32*:        decode_32,\
    u64*:        decode_64,\
    double*:     decode_double,\
    float*:      decode_float,\
    void**:      decode_ptr,\
    malc_lit*:   decode_lit,\
    malc_strcp*: decode_strcp,\
    malc_memcp*: decode_memcp,\
    default:     wrong\
    )\
  ((ch), (mem), (mem_end), (val))
#else

#endif
/*----------------------------------------------------------------------------*/
#ifdef MALC_NO_COMPRESSION
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
    se->t = bl_get_tstamp();
  }
}
/*----------------------------------------------------------------------------*/
#else /* MALC_NO_COMPRESSION */
/*----------------------------------------------------------------------------*/
void serializer_init(
  serializer* se, malc_const_entry const* entry, bool has_tstamp
  )
{
  se->entry      = entry;
  se->has_tstamp = has_tstamp;
#if BL_WORDSIZE == 64
  se->comp_entry = malc_get_compressed_u64 ((u64) entry);
#else
  se->comp_entry = malc_get_compressed_u32 ((u32) entry);
#endif
  if (has_tstamp) {
    se->t = malc_get_compressed_u64 ((u64) bl_get_tstamp());
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
#endif /* MALC_NO_COMPRESSION */
/*----------------------------------------------------------------------------*/
uword serializer_execute (serializer* se, u8* mem, va_list vargs)
{
  char const* partype = &se->entry->info[1];

  u8* wptr = mem;
#ifdef MALC_NO_COMPRESSION
  wptr = encode (se->ch, wptr, (void*) se->entry);
#else /* MALC_NO_COMPRESSION */
  *wptr       = 0;
  se->ch->hdr = wptr;
  se->ch->idx = 0;
  wptr = encode (se->ch, wptr + 1, se->comp_entry);
  se->ch->idx = 0;
  se->ch->hdr = wptr;
  memset (wptr, 0, serializer_hdr_size (se));
  wptr += serializer_hdr_size (se);
#endif /* MALC_NO_COMPRESSION */
  if (se->has_tstamp) {
    wptr = encode (se->ch, wptr, se->t);
  }
  /* the compiler will remove the fixed-size memcpy calls*/
  while (*partype) {
    switch (*partype) {
    case malc_type_i8:
    case malc_type_u8: {
      u8 v = malc_get_va_arg (vargs, v);
      wptr = encode (se->ch, wptr, v);
      break;
      }
    case malc_type_i16:
    case malc_type_u16: {
      u16 v = malc_get_va_arg (vargs, v);
      wptr = encode (se->ch, wptr, v);
      break;
      }
    case malc_type_i32:
    case malc_type_u32: {
#ifdef MALC_NO_BUILTIN_COMPRESSION
      u32 v = malc_get_va_arg (vargs, v);
#else
      malc_compressed_32 v = malc_get_va_arg (vargs, v);
#endif
      wptr = encode (se->ch, wptr, v);
      break;
      }
    case malc_type_float: {
      float v = malc_get_va_arg (vargs, v);
      wptr = encode (se->ch, wptr, v);
      break;
      }
    case malc_type_i64:
    case malc_type_u64: {
#ifdef MALC_NO_BUILTIN_COMPRESSION
      u64 v = malc_get_va_arg (vargs, v);
#else
      malc_compressed_64 v = malc_get_va_arg (vargs, v);
#endif
      wptr = encode (se->ch, wptr, v);
      break;
      }
    case malc_type_double: {
      double v = malc_get_va_arg (vargs, v);
      wptr = encode (se->ch, wptr, v);
      break;
      }
    case malc_type_ptr: {
#ifdef MALC_NO_PTR_COMPRESSION
      void* v = malc_get_va_arg (vargs, v);
#elif BL_WORDSIZE == 64
      malc_compressed_64 v = malc_get_va_arg (vargs, v);
#else
      malc_compressed_32 v = malc_get_va_arg (vargs, v);
#endif
      wptr = encode (se->ch, wptr, v);
      break;
      }
    case malc_type_lit: {
#ifdef MALC_NO_PTR_COMPRESSION
      malc_lit v = malc_get_va_arg (vargs, v);
#elif BL_WORDSIZE == 64
      malc_compressed_64 v = malc_get_va_arg (vargs, v);
#else
      malc_compressed_32 v = malc_get_va_arg (vargs, v);
#endif
      wptr = encode (se->ch, wptr, v);
      break;
      }
    case malc_type_strcp:
    case malc_type_memcp: {
      malc_memcp v = malc_get_va_arg (vargs, v);
      if (unlikely (!v.mem)) {
        v.size = 0;
      }
      wptr = encode (se->ch, wptr, v);
      break;
      }
    default: {
      bl_assert (0 && "bug");
      }
    }
    ++partype;
  }
  return (uword) (wptr - mem);
}
/*----------------------------------------------------------------------------*/
bl_err deserializer_init (deserializer* ds, alloc_tbl const* alloc)
{
  memset (ds, 0, sizeof *ds);
#ifdef MALC_NO_COMPRESSION
  ds->ch = nullptr;
#else
  ds->ch = &ds->chval;
#endif
  return log_args_init (&ds->args, 16, alloc);
}
/*----------------------------------------------------------------------------*/
void deserializer_destroy (deserializer* ds, alloc_tbl const* alloc)
{
  log_args_destroy (&ds->args, alloc);
}
/*----------------------------------------------------------------------------*/
void deserializer_reset (deserializer* ds)
{
  log_args_drop_tail_n (&ds->args, log_args_size (&ds->args));
  ds->entry   = nullptr;
#ifndef MALC_NO_COMPRESSION
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
#ifdef MALC_NO_COMPRESSION
  bl_err err = decode (ds->ch, &mem, mem_end, (void**) &ds->entry);
  if (unlikely (err)) {
    return err;
  }
#else
  ds->entry   = nullptr;
  ds->ch->hdr = mem;
  ++mem;
#if BL_WORDSIZE == 64
  bl_err err = decode_compressed_64 (ds->ch, &mem, mem_end, (u64*) &ds->entry);
#else
  bl_err err = decode_compressed_32 (ds->ch, &mem, mem_end, (u32*) &ds->entry);
#endif
  if (unlikely (err)) {
    return err;
  }
  ds->ch->hdr = mem;
  ds->ch->idx = 0;
  mem        += div_ceil (ds->entry->compressed_count + has_timestamp, 2);
#endif
  if (has_timestamp) {
    ds->t = 0;
    err   = decode (ds->ch, &mem, mem_end, &ds->t);
    if (unlikely (err)) {
      return err;
    }
  }
  else {
    ds->t = bl_get_tstamp();
  }
  char const* partype = &ds->entry->info[1];
  log_argument larg;

  while (*partype) {
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
    default:
      bl_assert (0 && "bug");
      return bl_invalid;
      break;
    } /* switch */
    if (!err) {
      err = log_args_insert_tail (&ds->args, &larg, alloc);
    }
    if (unlikely (err)) {
      return err;
    }
    ++partype;
  }
  return err;
}
/*----------------------------------------------------------------------------*/
