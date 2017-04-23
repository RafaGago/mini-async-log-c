#include <malc/malc.h>

/*----------------------------------------------------------------------------*/
struct malc {

};
/*----------------------------------------------------------------------------*/
MALC_EXPORT uword malc_get_size (void)
{
  return sizeof (malc);
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_create (malc** l)
{
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_destroy (malc* l)
{
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_get_cfg (malc* l, malc_cfg* cfg)
{
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_init (malc* l, malc_cfg const* cfg)
{
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_terminate (malc* l)
{
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_producer_thread_local_init (malc* l, u32 bytes)
{
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_run_consume_task (malc* l, uword timeout_us)
{
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_run_idle_task (malc* l, bool force)
{
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_add_destination(
  malc* l, u32* dst_id, malc_dst const* dst
  )
{
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_get_destination(
  malc* l, u32 dst_id, malc_dst* dst, void* instance
  )
{
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_set_destination_severity(
  malc* l, u32 dst_id, u8 severity
  )
{
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT uword malc_get_min_severity (void const* malc_ptr)
{

}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_log(
  void* malc_ptr,
  const malc_const_entry* e,
  uword min_size,
  uword max_size,
  int   argc,
  ...
  )
{
  bl_assert (e && e->format && e->info);

  bl_err err = bl_ok;
  va_list vargs;
  va_start (vargs, argc);

  /*at this point vargs may be passed to the encoder through vargs*/

  char const* partype = &e->info[1];

  while (*partype) {
    switch (*partype) {
    case malc_type_float: {
      float v = (float) va_arg (vargs, double);
      printf ("float: %f\n", v);
      break;
      }
    case malc_type_double: {
      double v = (double) va_arg (vargs, double);
      printf ("double: %f\n", v);
      break;
      }
    case malc_type_i8: {
      i8 v = (i8) va_arg (vargs, int);
      printf ("i8: %"PRId8"\n", v);
      break;
      }
    case malc_type_u8: {
      u8 v = (u8) va_arg (vargs, int);
      printf ("u8: %"PRIu8"\n", v);
      break;
      }
    case malc_type_i16: {
      i16 v = (i16) va_arg (vargs, int);
      printf ("i16: %"PRId16"\n", v);
      break;
      }
    case malc_type_u16: {
      u16 v = (u16) va_arg (vargs, int);
      printf ("u16: %"PRIu16"\n", v);
      break;
      }
#ifdef BL_32
    case malc_type_i32: {
      i32 v = (i32) va_arg (vargs, i32);
      printf ("i32: %"PRId32"\n", v);
      break;
      }
    case malc_type_u32: {
      u32 v = (u32)  va_arg (vargs, u32);
      printf ("u32: %"PRIu32"\n", v);
      break;
      }
#else
      case malc_type_i32: {
      i32 v = (i32) va_arg (vargs, int);
      printf ("i32: %"PRId32"\n", v);
      break;
      }
    case malc_type_u32: {
      u32 v = (u32) va_arg (vargs, int);
      printf ("u32: %"PRIu32"\n", v);
      break;
      }
#endif
      case malc_type_i64: {
      i64 v = (i64) va_arg (vargs, i64);
      printf ("i64: %"PRId64"\n", v);
      break;
      }
    case malc_type_u64: {
      u64 v = (u64) va_arg (vargs, u64);
      printf ("u64: %"PRIu64"\n", v);
      break;
      }
    case malc_type_vptr: {
      void* v = va_arg (vargs, void*);
      printf ("vptr: 0x%08lx\n", (u64) v);
      break;
      }
    case malc_type_lit: {
      malc_lit v = va_arg (vargs, malc_lit);
      printf ("string literal: %s\n", v.lit);
      break;
      }
    case malc_type_str: {
      malc_str v = va_arg (vargs, malc_str);
      printf ("string: len: %d, %s\n", (int) v.len, v.str);
      break;
      }
    case malc_type_bytes: {
      malc_mem  v = va_arg (vargs, malc_mem);
      printf ("mem: len: %d, ptr: 0x%08lx\n", v.size, (u64) v.mem);
      break;
      }
    default: {
      printf ("invalid\n");
      err = bl_invalid;
      goto end_process_loop;
      }
    }
    ++partype;
  }
end_process_loop:
  va_end (vargs);
  return err;
}
/*----------------------------------------------------------------------------*/