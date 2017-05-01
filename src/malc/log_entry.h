#ifndef __MALC_LOG_ENTRY_H__
#define __MALC_LOG_ENTRY_H__

#include <string.h>

#include <bl/base/platform.h>
#include <bl/base/integer.h>
#include <bl/base/time.h>
#include <bl/base/static_integer_math.h>

#include <malc/malc.h>

#ifdef MALC_NO_BUILTIN_COMPRESSION
/*----------------------------------------------------------------------------*/
typedef struct entry_encoder {
  malc_const_entry const* entry;
  bool                    has_tstamp;
  tstamp                  t;
  uword                   extra_size;
}
entry_encoder;
/*----------------------------------------------------------------------------*/
static void entry_encoder_init(
  entry_encoder* ee, malc_const_entry const* entry, bool has_tstamp
  )
{
  ee->entry      = entry;
  ee->has_tstamp = has_tstamp;
  ee->extra_size = sizeof ee->entry + has_tstamp ? sizeof e->t : 0;
  ee->hdr_size   = 0;
  if (has_tstamp) {
    ee->t = bl_get_tstamp();
  }
}
/*----------------------------------------------------------------------------*/
static uword entry_encoder_hdr_size (entry_encoder const* ee)
{
  return 0;
}
/*----------------------------------------------------------------------------*/
#else /* MALC_NO_BUILTIN_COMPRESSION */
/*----------------------------------------------------------------------------*/
typedef struct entry_encoder {
  malc_const_entry const* entry;
  bool                    has_tstamp;
#if BL_WORDSIZE == 32
  malc_compressed_32      comp_entry;
#elif BL_WORDSIZE == 64
  malc_compressed_64      comp_entry;
#else
  #error "unsoported bit width"
#endif
  malc_compressed_64      t;
  uword                   hdr_size;
  uword                   extra_size;
  uword                   compress_idx;
}
entry_encoder;
/*----------------------------------------------------------------------------*/
static void entry_encoder_init(
  entry_encoder* ee, malc_const_entry const* entry, bool has_tstamp
  )
{
  ee->entry      = entry;
  ee->has_tstamp = has_tstamp;
#if BL_WORDSIZE == 64
  ee->comp_entry = malc_get_compressed_u64 ((u64) entry);
#else
  ee->comp_entry = malc_get_compressed_u32 ((u32) entry);
#endif
  if (has_tstamp) {
    ee->t = malc_get_compressed_u64 ((u64) bl_get_tstamp());
  }
  ee->hdr_size   = div_ceil (1 + has_tstamp + entry->compressed_count, 2);
  ee->extra_size = (has_tstamp ? (ee->t.format_nibble & 7) : 0) +
    (ee->comp_entry.format_nibble & 7);
  ee->compress_idx = 0;
}
/*----------------------------------------------------------------------------*/
static uword entry_encoder_hdr_size (entry_encoder const* ee)
{
  return ee->hdr_size;
}
/*----------------------------------------------------------------------------*/
#endif /* MALC_NO_BUILTIN_COMPRESSION */
#ifdef MALC_NO_BUILTIN_COMPRESSION
/*----------------------------------------------------------------------------*/
static u8* serialize_32 (entry_encoder* ee, u8* mem, u8* wptr, u32 v)
{
  memcpy (wptr, &v, sizeof v);
  return wptr + sizeof v;
}
/*----------------------------------------------------------------------------*/
static u8* serialize_64 (entry_encoder* ee, u8* mem, u8* wptr, u64 v)
{
  memcpy (wptr, &v, sizeof v);
  return wptr + sizeof v;
}
#else /* MALC_NO_BUILTIN_COMPRESSION */
/*----------------------------------------------------------------------------*/
static u8* serialize_32(
  entry_encoder* ee, u8* mem, u8* wptr, malc_compressed_32 v
  )
{
  uword size = (v.format_nibble & ((1 << 3) - 1)) + 1;
  bl_assert (size <= sizeof (u32));
  mem[ee->compress_idx / 2] |=
    (u8) v.format_nibble << ((ee->compress_idx & 1) * 4);
  ++ee->compress_idx;
  for (uword i = 0; i < size; ++i) {
    *wptr = (u8) (v.v >> (i * 8));
    ++wptr;
  }
  return wptr;
}
/*----------------------------------------------------------------------------*/
static u8* serialize_64(
  entry_encoder* ee, u8* mem, u8* wptr, malc_compressed_64 v
  )
{
  uword size = (v.format_nibble & ((1 << 3) - 1)) + 1;
  bl_assert (size <= sizeof (u64));
  mem[ee->compress_idx / 2] |=
    (u8) v.format_nibble << ((ee->compress_idx & 1) * 4);
  ++ee->compress_idx;
  for (uword i = 0; i < size; ++i) {
    *wptr = (u8) (v.v >> (i * 8));
    ++wptr;
  }
  return wptr;
}
#endif /* MALC_NO_BUILTIN_COMPRESSION */
/*----------------------------------------------------------------------------*/
static uword entry_encoder_entry_size (entry_encoder const* ee, uword payload)
{
  return payload + entry_encoder_hdr_size (ee) + ee->extra_size;
}
/*----------------------------------------------------------------------------*/
static void entry_encoder_execute (entry_encoder* ee, u8* mem, va_list vargs)
{
  char const* partype = &ee->entry->info[1];

  u8* wptr = mem;
  for (uword i = 0; i < entry_encoder_hdr_size (ee); ++i) {
    wptr[i] = 0;
    ++wptr;
  }
#ifdef MALC_NO_BUILTIN_COMPRESSION
  memcpy (wptr, ee->entry, sizeof ee->entry);
  wptr += sizeof ee->entry;
  if (ee->has_tstamp) {
    memcpy (wptr, ee->t, sizeof ee->t);
    wptr += sizeof ee->t;
  }
#else /* MALC_NO_BUILTIN_COMPRESSION */
#if BL_WORDSIZE == 64
  wptr = serialize_64 (ee, mem, wptr, ee->comp_entry);
#else
  wptr = serialize_32 (ee, mem, wptr, ee->comp_entry);
#endif
  if (ee->has_tstamp) {
    wptr = serialize_64 (ee, mem, wptr, ee->t);
  }
#endif /* MALC_NO_BUILTIN_COMPRESSION */
  /* the compiler will remove the fixed-size memcpy calls*/
  while (*partype) {
    switch (*partype) {
    case malc_type_i8:
    case malc_type_u8: {
      u8 v = malc_get_va_arg (vargs, v);
      memcpy (wptr, &v, sizeof v);
      wptr += sizeof v;
      break;
      }
    case malc_type_i16:
    case malc_type_u16: {
      u16 v = malc_get_va_arg (vargs, v);
      memcpy (wptr, &v, sizeof v);
      wptr += sizeof v;
      break;
      }
    case malc_type_i32:
    case malc_type_u32: {
#ifdef MALC_NO_BUILTIN_COMPRESSION
      u32 v = malc_get_va_arg (vargs, v);
#else
      malc_compressed_32 v = malc_get_va_arg (vargs, v);
#endif
      wptr = serialize_32 (ee, mem, wptr, v);
      break;
      }
    case malc_type_float: {
      float v = malc_get_va_arg (vargs, v);
      memcpy (wptr, &v, sizeof v);
      wptr += sizeof v;
      break;
      }
    case malc_type_i64:
    case malc_type_u64: {
#ifdef MALC_NO_BUILTIN_COMPRESSION
      u64 v = malc_get_va_arg (vargs, v);
#else
      malc_compressed_64 v = malc_get_va_arg (vargs, v);
#endif
      wptr = serialize_64 (ee, mem, wptr, v);
      break;
      }
    case malc_type_double: {
      double v = malc_get_va_arg (vargs, v);
      memcpy (wptr, &v, sizeof v);
      wptr += sizeof v;
      break;
      }
    case malc_type_ptr: {
      void* v = malc_get_va_arg (vargs, v);
      memcpy (wptr, &v, sizeof v);
      wptr += sizeof v;
      break;
      }
    case malc_type_lit: {
      malc_lit v = malc_get_va_arg (vargs, v);
      memcpy (wptr, &v.lit, sizeof v);
      wptr += sizeof v;
      break;
      }
    case malc_type_str:
    case malc_type_bytes: {
      malc_mem v = malc_get_va_arg (vargs, v);
      if (unlikely (!v.mem)) {
        v.size = 0;
      }
      memcpy (wptr, &v.size, sizeof v.size);
      wptr += sizeof v.size;
      memcpy (wptr, v.mem, v.size);
      wptr += v.size;
      break;
      }
    default: {
      bl_assert (0 && "bug");
      }
    }
    ++partype;
  }
}
/*----------------------------------------------------------------------------*/

#endif