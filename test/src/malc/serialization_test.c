#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <bl/base/utility.h>
#include <bl/base/default_allocator.h>
#include <bl/cmocka_pre.h>

#include <malc/malc.h>
#include <malc/alltypes.h>
#include <malc/serialization.h>

/*----------------------------------------------------------------------------*/
#define SER_TEST_GET_ENTRY(var, ...)\
  MALC_LOG_CREATE_CONST_ENTRY (malc_sev_warning, "", __VA_ARGS__); \
  var = &bl_pp_tokconcat(malc_const_entry_, __LINE__)
/*----------------------------------------------------------------------------*/
#define MALC_LOG_TEST_DECLARE_TMP_VARIABLES(...)\
  MALC_LOG_DECLARE_TMP_VARIABLES_ASSIGN_REFTYPES (__VA_ARGS__)\
  MALC_LOG_TMP_VARIABLES_ASSIGN_NON_REFTYPES (__VA_ARGS__)
/*----------------------------------------------------------------------------*/
typedef struct ser_deser_context {
  bl_u8        buff[2048];
  deserializer deser;
  bl_alloc_tbl alloc;
}
ser_deser_context;
/*----------------------------------------------------------------------------*/
static malc_serializer get_external_serializer(
  ser_deser_context* c, malc_const_entry const* entry
  )
{
  serializer ser;
  serializer_init (&ser, entry, false);
  /*Buff is assumed to be big enough, "serializer_log_entry_size" is called on
  the impl*/
  return serializer_prepare_external_serializer (&ser, c->buff, c->buff);
}
/*----------------------------------------------------------------------------*/
static int ser_test_setup (void **state)
{
  static ser_deser_context c;
  memset (c.buff, -1, sizeof c.buff);
  c.alloc = bl_get_default_alloc();
  assert_int_equal (deserializer_init (&c.deser, &c.alloc).own, bl_ok);
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
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, le.args_count);
  assert_int_equal (0, le.refs_count);
  assert_int_equal (nullptr, le.refdtor.func);
  assert_true (le.args[0].vfloat == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_double (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  double v = 123451231234.12341231235;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, le.args_count);
  assert_int_equal (0, le.refs_count);
  assert_int_equal (nullptr, le.refdtor.func);
  assert_true (le.args[0].vdouble == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_i8 (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  bl_i8 v = -92;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, le.args_count);
  assert_int_equal (0, le.refs_count);
  assert_int_equal (nullptr, le.refdtor.func);
  assert_true (le.args[0].vi8 == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_i16 (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  bl_i16 v = -92 * 255;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, le.args_count);
  assert_int_equal (0, le.refs_count);
  assert_int_equal (nullptr, le.refdtor.func);
  assert_true (le.args[0].vi16 == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_i32 (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  bl_i32 v = -92 * 255 * 255 * 255;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  MALC_LOG_TEST_DECLARE_TMP_VARIABLES (v);
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, I);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, le.args_count);
  assert_int_equal (0, le.refs_count);
  assert_int_equal (nullptr, le.refdtor.func);
  assert_true (le.args[0].vi32 == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_i64 (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  bl_i64 v = -92 * ((bl_u64) 1 << 58);
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  MALC_LOG_TEST_DECLARE_TMP_VARIABLES (v);
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, I);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, le.args_count);
  assert_int_equal (0, le.refs_count);
  assert_int_equal (nullptr, le.refdtor.func);
  assert_true (le.args[0].vi64 == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_u8 (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  bl_u8 v = 92;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, le.args_count);
  assert_int_equal (0, le.refs_count);
  assert_int_equal (nullptr, le.refdtor.func);
  assert_true (le.args[0].vu8 == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_u16 (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  bl_u16 v = 92 * 255;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, le.args_count);
  assert_int_equal (0, le.refs_count);
  assert_int_equal (nullptr, le.refdtor.func);
  assert_true (le.args[0].vu16 == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_u32 (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  bl_u32 v = 92 * 255 * 255 * 255;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  MALC_LOG_TEST_DECLARE_TMP_VARIABLES (v);
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, I);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, le.args_count);
  assert_int_equal (0, le.refs_count);
  assert_int_equal (nullptr, le.refdtor.func);
  assert_true (le.args[0].vu32 == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_u64 (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  bl_u64 v = 92 * ((bl_u64) 1 << 58);
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  MALC_LOG_TEST_DECLARE_TMP_VARIABLES (v);
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, I);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, le.args_count);
  assert_int_equal (0, le.refs_count);
  assert_int_equal (nullptr, le.refdtor.func);
  assert_true (le.args[0].vu64 == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_ptr (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  void* v = (void*) 0xaa00aa00;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  MALC_LOG_TEST_DECLARE_TMP_VARIABLES (v);
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, I);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, le.args_count);
  assert_int_equal (0, le.refs_count);
  assert_int_equal (nullptr, le.refdtor.func);
  assert_true (le.args[0].vptr == v);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_lit (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  malc_lit v = {(char const*) 0xaa00aa00 };
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  MALC_LOG_TEST_DECLARE_TMP_VARIABLES (v);
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, I);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, le.args_count);
  assert_int_equal (0, le.refs_count);
  assert_int_equal (nullptr, le.refdtor.func);
  assert_true (le.args[0].vlit.lit == v.lit);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_strcp (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  char str[] = "eaaaa!";
  malc_strcp v = { str, bl_lit_len (str) };
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, le.args_count);
  assert_int_equal (0, le.refs_count);
  assert_int_equal (nullptr, le.refdtor.func);
  assert_memory_equal (le.args[0].vstrcp.str, str, bl_lit_len (str));
  assert_int_equal (le.args[0].vstrcp.len, bl_lit_len (str));
}
/*----------------------------------------------------------------------------*/
static void serialization_test_strref (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  malc_strref v  = { (char*) 0x123456, 12345 };
  malc_refdtor d = { (malc_refdtor_fn) 0x145645, (void*) 0x3434 };
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v, d);
  MALC_LOG_TEST_DECLARE_TMP_VARIABLES (v, d);
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, I);
  malc_serialize (&ser, II);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, le.args_count);
  assert_int_equal (1, le.refs_count);
  assert_int_equal (le.args[0].vstrref.len, v.len);
  assert_ptr_equal (le.args[0].vstrref.str, v.str);
  assert_int_equal (le.refs[0].size, v.len);
  assert_ptr_equal (le.refs[0].ref, v.str);
  assert_int_equal (le.refdtor.func, d.func);
  assert_ptr_equal (le.refdtor.context, d.context);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_memcp (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  char str[] = "eaaaeaa!";
  malc_memcp v = { (bl_u8*) str, sizeof str };
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, le.args_count);
  assert_int_equal (0, le.refs_count);
  assert_int_equal (nullptr, le.refdtor.func);
  assert_string_equal ((char const*) le.args[0].vmemcp.mem, str);
  assert_int_equal (le.args[0].vmemcp.size, sizeof str);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_memref (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  malc_memref v  = { (bl_u8*) 0x123456, 12345 };
  malc_refdtor d = { (malc_refdtor_fn) 0x145645, (void*) 0x3434 };
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v, d);
  MALC_LOG_TEST_DECLARE_TMP_VARIABLES (v, d);
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, I);
  malc_serialize (&ser, II);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  assert_int_equal (1, le.args_count);
  assert_int_equal (1, le.refs_count);
  assert_int_equal (le.args[0].vmemref.size, v.size);
  assert_ptr_equal (le.args[0].vmemref.mem, v.mem);
  assert_int_equal (le.refs[0].size, v.size);
  assert_ptr_equal (le.refs[0].ref, v.mem);
  assert_int_equal (le.refdtor.func, d.func);
  assert_ptr_equal (le.refdtor.context, d.context);
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
  all.vstrcp.str   = "str";
  all.vstrcp.len   = sizeof "str";
  all.vmemcp.mem   = (bl_u8 const*) "mem";
  all.vmemcp.size  = sizeof "mem";
  all.vstrref.str  = "strref";
  all.vstrref.len  = sizeof "strref";
  all.vmemref.mem  = (bl_u8*) "memref";
  all.vmemref.size = sizeof "memref";
  malc_refdtor d = { (malc_refdtor_fn) 0x145645, (void*) 0x3434 };

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
    all.vstrref,
    all.vmemref,
    d
    );
  MALC_LOG_TEST_DECLARE_TMP_VARIABLES(
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
    all.vstrref,
    all.vmemref,
    d
    );
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, I);
  malc_serialize (&ser, II);
  malc_serialize (&ser, III);
  malc_serialize (&ser, IIII);
  malc_serialize (&ser, IIIII);
  malc_serialize (&ser, IIIIII);
  malc_serialize (&ser, IIIIIII);
  malc_serialize (&ser, IIIIIIII);
  malc_serialize (&ser, IIIIIIIII);
  malc_serialize (&ser, IIIIIIIIII);
  malc_serialize (&ser, IIIIIIIIIII);
  malc_serialize (&ser, IIIIIIIIIIII);
  malc_serialize (&ser, IIIIIIIIIIIII);
  malc_serialize (&ser, IIIIIIIIIIIIII);
  malc_serialize (&ser, IIIIIIIIIIIIIII);
  malc_serialize (&ser, IIIIIIIIIIIIIIII);
  malc_serialize (&ser, IIIIIIIIIIIIIIIII);

  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof c->buff, false, &c->alloc
    );
  assert_int_equal (err.own, bl_ok);
  log_entry le = deserializer_get_log_entry (&c->deser);
  assert_ptr_equal (entry, le.entry);
  bl_uword idx = 0;

  assert_int_equal (le.args[idx].vu8, all.vu8);
  ++idx;
  assert_int_equal (le.args[idx].vi8, all.vi8);
  ++idx;
  assert_int_equal (le.args[idx].vu16, all.vu16);
  ++idx;
  assert_int_equal (le.args[idx].vi16, all.vi16);
  ++idx;
  assert_true (le.args[idx].vu32 == all.vu32);
  ++idx;
  assert_true (le.args[idx].vi32 == all.vi32);
  ++idx;
  assert_true (le.args[idx].vu64 == all.vu64);
  ++idx;
  assert_true (le.args[idx].vi64 == all.vi64);
  ++idx;
  assert_true (le.args[idx].vfloat == all.vfloat);
  ++idx;
  assert_true (le.args[idx].vdouble == all.vdouble);
  ++idx;
  assert_true (le.args[idx].vptr == all.vptr);
  ++idx;
  assert_true (le.args[idx].vlit.lit == all.vlit.lit);
  ++idx;
  assert_string_equal (le.args[idx].vstrcp.str, all.vstrcp.str);
  assert_int_equal (le.args[idx].vstrcp.len, all.vstrcp.len);
  ++idx;
  assert_string_equal ((char const*) le.args[idx].vmemcp.mem, all.vmemcp.mem);
  assert_int_equal (le.args[idx].vmemcp.size, all.vmemcp.size);
  ++idx;
  assert_string_equal (le.args[idx].vstrref.str, all.vstrref.str);
  assert_int_equal (le.args[idx].vstrref.len, all.vstrref.len);
  ++idx;
  assert_string_equal ((char const*) le.args[idx].vmemref.mem, all.vmemref.mem);
  assert_int_equal (le.args[idx].vmemref.size, all.vmemref.size);
  ++idx;
  assert_int_equal (idx, le.args_count);
  assert_int_equal (2, le.refs_count);
  assert_ptr_equal (le.refdtor.func, d.func);
  assert_ptr_equal (le.refdtor.context, d.context);
}
/*----------------------------------------------------------------------------*/
static void serialization_test_small_buffer (void **state)
{
  ser_deser_context* c = (ser_deser_context*) *state;
  bl_u16 v = 92 * 255;
  malc_const_entry const* entry;
  SER_TEST_GET_ENTRY (entry, v);
  malc_serializer ser = get_external_serializer (c, entry);
  malc_serialize (&ser, v);
  deserializer_reset (&c->deser);
  bl_err err = deserializer_execute(
    &c->deser, c->buff, c->buff + sizeof v, false, &c->alloc
    );
  assert_int_equal (err.own, bl_invalid);
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
    serialization_test_strcp, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_strref, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_memcp, ser_test_setup, ser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    serialization_test_memref, ser_test_setup, ser_test_teardown
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
