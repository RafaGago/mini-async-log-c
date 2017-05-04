#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <bl/base/utility.h>
#include <bl/base/default_allocator.h>
#include <bl/cmocka_pre.h>

#include <malc/malc.h>
#include <malc/alltypes.h>
#include <malc/serialization.h>

#define SER_TEST_GET_ENTRY(var, ...)\
  MALC_LOG_CREATE_CONST_ENTRY (malc_sev_warning, "", __VA_ARGS__); \
  var = &pp_tokconcat(malc_const_entry_, __LINE__)

/*----------------------------------------------------------------------------*/
typedef struct ser_deser_context {
  u8           buff[1024];
  deserializer deser;
  alloc_tbl    alloc;
}
ser_deser_context;
/*----------------------------------------------------------------------------*/
static void serialize_to_buffer(
  ser_deser_context* c, malc_const_entry const* entry, ...
  )
{
  serializer ser;
  serializer_init (&ser, entry, false);
  va_list vargs;
  va_start (vargs, entry);
  assert_true (serializer_execute (&ser, c->buff, vargs) > 0);
  va_end (vargs);
}
/*----------------------------------------------------------------------------*/
static int ser_test_setup (void **state)
{
  static ser_deser_context c;
  memset (c.buff, -1, sizeof c.buff);
  c.alloc = get_default_alloc();
  assert_int_equal (deserializer_init (&c.deser, &c.alloc), bl_ok);
  *state = &c;
  return 0;
}
/*----------------------------------------------------------------------------*/
static int ser_test_teardown (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  deserializer_destroy (&c->deser, &c->alloc);
  return 0;
}
/*----------------------------------------------------------------------------*/
static void serialization_test_float (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  float v = 12345.12345f;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  serialize_to_buffer (c, entry, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, log_args_size (le.args));
  assert_true (log_args_at (le.args, 0)->vfloat == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_double (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  double v = 123451231234.12341231235;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  serialize_to_buffer (c, entry, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, log_args_size (le.args));
  assert_true (log_args_at (le.args, 0)->vdouble == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_i8 (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  i8 v = -92;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  serialize_to_buffer (c, entry, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, log_args_size (le.args));
  assert_true (log_args_at (le.args, 0)->vi8 == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_i16 (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  i16 v = -92 * 255;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  serialize_to_buffer (c, entry, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, log_args_size (le.args));
  assert_true (log_args_at (le.args, 0)->vi16 == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_i32 (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  i32 v = -92 * 255 * 255 * 255;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  MALC_LOG_DECLARE_TMP_VARIABLES (v);
  serialize_to_buffer (c, entry, I);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, log_args_size (le.args));
  assert_true (log_args_at (le.args, 0)->vi32 == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_i64 (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  i64 v = -92 * ((u64) 1 << 58);
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  MALC_LOG_DECLARE_TMP_VARIABLES (v);
  serialize_to_buffer (c, entry, I);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, log_args_size (le.args));
  assert_true (log_args_at (le.args, 0)->vi64 == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_u8 (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  u8 v = 92;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  serialize_to_buffer (c, entry, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, log_args_size (le.args));
  assert_true (log_args_at (le.args, 0)->vu8 == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_u16 (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  u16 v = 92 * 255;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  serialize_to_buffer (c, entry, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, log_args_size (le.args));
  assert_true (log_args_at (le.args, 0)->vu16 == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_u32 (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  u32 v = 92 * 255 * 255 * 255;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  MALC_LOG_DECLARE_TMP_VARIABLES (v);
  serialize_to_buffer (c, entry, I);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, log_args_size (le.args));
  assert_true (log_args_at (le.args, 0)->vu32 == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_u64 (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  u64 v = 92 * ((u64) 1 << 58);
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  MALC_LOG_DECLARE_TMP_VARIABLES (v);
  serialize_to_buffer (c, entry, I);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, log_args_size (le.args));
  assert_true (log_args_at (le.args, 0)->vu64 == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_ptr (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  void* v = (void*) 0xaa00aa00;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  MALC_LOG_DECLARE_TMP_VARIABLES (v);
  serialize_to_buffer (c, entry, I);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, log_args_size (le.args));
  assert_true (log_args_at (le.args, 0)->vptr == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_lit (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  malc_lit v = {(char const*) 0xaa00aa00 };
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  MALC_LOG_DECLARE_TMP_VARIABLES (v);
  serialize_to_buffer (c, entry, I);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, log_args_size (le.args));
  assert_true (log_args_at (le.args, 0)->vlit.lit == v.lit);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_str (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  char str[] = "eaaaa!";
  malc_strcp v = { str, lit_len (str) };
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  serialize_to_buffer (c, entry, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, log_args_size (le.args));
  assert_memory_equal(
    log_args_at (le.args, 0)->vstrcp.str, str, lit_len (str)
    );
  assert_int_equal (log_args_at (le.args, 0)->vstrcp.len, lit_len (str));
}
/*----------------------------------------------------------------------------*/
static void serialization_test_bytes (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  char str[] = "eaaaeaa!";
  malc_memcp v = { (u8*) str, sizeof str };
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  serialize_to_buffer (c, entry, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, log_args_size (le.args));
  assert_string_equal ((char const*) log_args_at (le.args, 0)->vmemcp.mem, str);
  assert_int_equal (log_args_at (le.args, 0)->vmemcp.size, sizeof str);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_all (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  alltypes all;
  all.vu8       = 0x11;
  all.vi8       = 0x22;
  all.vu16      = 0x3333;
  all.vi16      = 0x4444;
  all.vu32      = 0x55555555;
  all.vi32      = 0x66666666;
  all.vu64      = 0x7777777777777777;
  all.vi64      = 0x8888888888888888;
  all.vfloat    = 195953.2342f;
  all.vdouble   = 1231231123123123.234234444;
  all.vptr      = (void*) 0xaaaaaaaa;
  all.vlit.lit  = (char const*) 0xbbbbbbbb;
  all.vstrcp.str  = "str";
  all.vstrcp.len  = sizeof "str";
  all.vmemcp.mem  = (u8 const*) "mem";
  all.vmemcp.size = sizeof "mem";

  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY(
    entry,
    all.vu8,
    all.vi8,
    all.vu16,
    all.vi16,
    all.vu32,
    all.vi32,
    all.vu64,
    all.vi64,
    all.vfloat,
    all.vdouble,
    all.vptr,
    all.vlit,
    all.vstrcp,
    all.vmemcp,
    );
  MALC_LOG_DECLARE_TMP_VARIABLES(
    all.vu8,
    all.vi8,
    all.vu16,
    all.vi16,
    all.vu32,
    all.vi32,
    all.vu64,
    all.vi64,
    all.vfloat,
    all.vdouble,
    all.vptr,
    all.vlit,
    all.vstrcp,
    all.vmemcp,
    );
  serialize_to_buffer(
    c,
    entry,
    I,
    II,
    III,
    IIII,
    IIIII,
    IIIIII,
    IIIIIII,
    IIIIIIII,
    IIIIIIIII,
    IIIIIIIIII,
    IIIIIIIIIII,
    IIIIIIIIIIII,
    IIIIIIIIIIIII,
    IIIIIIIIIIIIII
    );
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  uword idx = 0;

  assert_int_equal (log_args_at (le.args, idx)->vu8, all.vu8);
  ++idx;
  assert_int_equal (log_args_at (le.args, idx)->vi8, all.vi8);
  ++idx;
  assert_int_equal (log_args_at (le.args, idx)->vu16, all.vu16);
  ++idx;
  assert_int_equal (log_args_at (le.args, idx)->vi16, all.vi16);
  ++idx;
  assert_true (log_args_at (le.args, idx)->vu32 == all.vu32);
  ++idx;
  assert_true (log_args_at (le.args, idx)->vi32 == all.vi32);
  ++idx;
  assert_true (log_args_at (le.args, idx)->vu64 == all.vu64);
  ++idx;
  assert_true (log_args_at (le.args, idx)->vi64 == all.vi64);
  ++idx;
  assert_true (log_args_at (le.args, idx)->vfloat == all.vfloat);
  ++idx;
  assert_true (log_args_at (le.args, idx)->vdouble == all.vdouble);
  ++idx;
  assert_true (log_args_at (le.args, idx)->vptr == all.vptr);
  ++idx;
  assert_true (log_args_at (le.args, idx)->vlit.lit == all.vlit.lit);
  ++idx;
  assert_string_equal (log_args_at (le.args, idx)->vstrcp.str, all.vstrcp.str);
  assert_int_equal (log_args_at (le.args, idx)->vstrcp.len, all.vstrcp.len);
  ++idx;
  assert_string_equal(
    (char const*) log_args_at (le.args, idx)->vmemcp.mem, all.vmemcp.mem
    );
  assert_int_equal (log_args_at (le.args, idx)->vmemcp.size, all.vmemcp.size);
  ++idx;
  assert_int_equal (idx, log_args_size (le.args));
}
/*----------------------------------------------------------------------------*/
static void serialization_test_small_buffer (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  u16 v = 92 * 255;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  serialize_to_buffer (c, entry, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof v, false, &c->alloc
    );
  assert_int_equal (err, bl_invalid);
}
/*----------------------------------------------------------------------------*/
static const struct CMUnitTest tests[] = {
  cmocka_unit_test_setup_teardown(
    serialization_test_float, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_double, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_i8, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_i16, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_i32, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_i64, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_u8, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_u16, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_u32, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_u64, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_ptr, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_lit, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_str, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_bytes, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_all, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_small_buffer, ser_test_setup, ser_test_teardown
    ),
};
/*----------------------------------------------------------------------------*/
int serialization_tests (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*----------------------------------------------------------------------------*/
