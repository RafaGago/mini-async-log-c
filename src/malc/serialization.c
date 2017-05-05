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
declare_autoarray_funcs (log_args, log_argument);
declare_autoarray_funcs (log_refs, malc_ref);
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
  uword size = malc_compressed_get_size (v.format_nibble);
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
  uword size = malc_compressed_get_size (v.format_nibble);
  ch->hdr[ch->idx / 2] |= (u8) v.format_nibble << ((ch->idx & 1) * 4);
  ++ch->idx;
  for (uword i = 0; i < size; ++i) {
    *mem = (u8) (v.v >> (i * 8));
    ++mem;
  }
  return mem;
}
/*----------------------------------------------------------------------------*/
#if BL_WORDSIZE == 64
  static u8* encode_compressed_ptr(
    compressed_header* ch, u8* mem, malc_compressed_ptr v
    )
  {
    return ENCODE_NAME_BUILD(_comp64) (ch, mem, v);
  }
#else
  static u8* encode_compressed_ptr(
    compressed_header* ch, u8* mem, malc_compressed_ptr v
    )
  {
    return ENCODE_NAME_BUILD(_comp32) (ch, mem, v);
  }
#endif
/*----------------------------------------------------------------------------*/
static inline u8* ENCODE_NAME_BUILD(_compref)(
  compressed_header* ch, u8* mem, malc_compressed_ref v
  )
{
  mem = ENCODE_NAME_BUILD(_16) (ch, mem, v.size);
  return encode_compressed_ptr (ch, mem, v.ref);
}
/*----------------------------------------------------------------------------*/
static inline u8* ENCODE_NAME_BUILD(_comprefdtor)(
  compressed_header* ch, u8* mem, malc_compressed_refdtor v
  )
{
  mem = encode_compressed_ptr (ch, mem, v.func);
  return encode_compressed_ptr (ch, mem, v.context);
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
/*lit come as compressed_ptr when !MALC_NO_PTR_COMPRESSION */
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
static inline u8* ENCODE_NAME_BUILD(_strref)(
  compressed_header* ch, u8* mem, malc_strref v
  )
{
  mem = ENCODE_NAME_BUILD(_16) (ch, mem, v.len);
  return ENCODE_NAME_BUILD(_ptr) (ch, mem, (void*) v.str);
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
static inline u8* ENCODE_NAME_BUILD(_memref)(
  compressed_header* ch, u8* mem, malc_memref v
  )
{
  mem = ENCODE_NAME_BUILD(_16) (ch, mem, v.size);
  return ENCODE_NAME_BUILD(_ptr) (ch, mem, (void*) v.mem);
}
/*----------------------------------------------------------------------------*/
static inline u8* ENCODE_NAME_BUILD(_refdtor)(
  compressed_header* ch, u8* mem, malc_refdtor v
  )
{
  mem = ENCODE_NAME_BUILD(_ptr) (ch, mem, (void*) v.func);
  return ENCODE_NAME_BUILD(_ptr) (ch, mem, (void*) v.context);
}
/*----------------------------------------------------------------------------*/
#ifndef __cplusplus
#define encode(ch, mem, val)\
  _Generic ((val),\
    u8:                      encode_8,\
    u16:                     encode_16,\
    u32:                     encode_32,\
    u64:                     encode_64,\
    double:                  encode_double,\
    float:                   encode_float,\
    void*:                   encode_ptr,\
    malc_compressed_32:      encode_comp32,\
    malc_compressed_64:      encode_comp64,\
    malc_compressed_ref:     encode_compref,\
    malc_compressed_refdtor: encode_comprefdtor,\
    malc_lit:                encode_lit,\
    malc_strcp:              encode_strcp,\
    malc_strref:             encode_strref,\
    malc_memcp:              encode_memcp,\
    malc_memref:             encode_memref,\
    malc_refdtor:            encode_refdtor,\
    default:                 wrong\
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
#if !defined (MALC_NO_COMPRESSION) && BL_WORDSIZE == 64
/*----------------------------------------------------------------------------*/
static bl_err decode_compressed_ptr(
  compressed_header* ch, u8** mem, u8* mem_end, void** v
  )
{
  return decode_compressed_64 (ch, mem, mem_end, (u64*) v);
}
/*----------------------------------------------------------------------------*/
#elif !defined (MALC_NO_COMPRESSION) && BL_WORDSIZE == 32
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
#else
/*----------------------------------------------------------------------------*/
static inline bl_err DECODE_NAME_BUILD(_ptr)(
  compressed_header* ch, u8** mem, u8* mem_end, void** v
  )
{
  return decode_compressed_ptr (ch, mem, mem_end, v);
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
static inline bl_err DECODE_NAME_BUILD(_strref)(
  compressed_header* ch, u8** mem, u8* mem_end, malc_strref* v
  )
{
  bl_err err = DECODE_NAME_BUILD(_16) (ch, mem, mem_end, &v->len);
  if (unlikely (err)) {
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
static inline bl_err DECODE_NAME_BUILD(_memref)(
  compressed_header* ch, u8** mem, u8* mem_end, malc_memref* v
  )
{
  bl_err err = DECODE_NAME_BUILD(_16) (ch, mem, mem_end, &v->size);
  if (unlikely (err)) {
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
  if (unlikely (err)) {
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
  se->comp_entry = malc_get_compressed_ptr ((void*) entry);
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
#else
      malc_compressed_ptr v = malc_get_va_arg (vargs, v);
#endif
      wptr = encode (se->ch, wptr, v);
      break;
      }
    case malc_type_lit: {
#ifdef MALC_NO_PTR_COMPRESSION
      malc_lit v = malc_get_va_arg (vargs, v);
#else
      malc_compressed_ptr v = malc_get_va_arg (vargs, v);
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
    case malc_type_strref:
    case malc_type_memref: {
#ifdef MALC_NO_PTR_COMPRESSION
      malc_memref v = malc_get_va_arg (vargs, v);
#else
      malc_compressed_ref v = malc_get_va_arg (vargs, v);
#endif
      wptr = encode (se->ch, wptr, v);
      break;
      }
    case malc_type_refdtor: {
#ifdef MALC_NO_PTR_COMPRESSION
      malc_refdtor v = malc_get_va_arg (vargs, v);
#else
      malc_compressed_refdtor v = malc_get_va_arg (vargs, v);
#endif
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
  bl_err err = log_args_init (&ds->args, 16, alloc);
  if (err) {
    return err;
  }
  err = log_refs_init (&ds->refs, 16, alloc);
  if (err) {
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
  bl_err err = decode_compressed_ptr(
    ds->ch, &mem, mem_end, (void**) &ds->entry
    );
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
      if (likely (!err)) {
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
      return bl_invalid;
      break;
    } /* switch */
    if (!err && push_this_arg) {
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
