#include <string.h>

#include <malc/destinations/array.h>

#include <bl/cmocka_pre.h>

#include <bl/base/error.h>
#include <bl/base/integer.h>
#include <bl/base/utility.h>

/*----------------------------------------------------------------------------*/
typedef struct array_dst_context {
  u64             instance_buff[16];
  char            entries[8][8];
  malc_array_dst* ad;
}
array_dst_context;
/*----------------------------------------------------------------------------*/
static const uword entry_len =
  sizeof_member (array_dst_context, entries[0]) - 1;
/*----------------------------------------------------------------------------*/
#define MALC_LOG_STRS_INITIALIZER(t, s, txt)\
  { t, lit_len (t), s, lit_len (s), txt, lit_len (txt) }
/*----------------------------------------------------------------------------*/
static int array_dst_test_setup (void **state)
{
  static array_dst_context c;
  assert_true (sizeof c.instance_buff >= malc_array_dst_tbl.size_of);
  c.ad       = (malc_array_dst*) c.instance_buff;
  bl_err err = malc_array_dst_tbl.init ((void*) c.ad, nullptr);
  assert_int_equal (bl_ok, err.bl);
  malc_array_dst_set_array(
    c.ad, (char*) c.entries, arr_elems (c.entries), arr_elems (c.entries[0])
    );
  assert_int_equal (arr_elems (c.entries) - 1, malc_array_dst_capacity (c.ad));
  *state = (void*) &c;
  return 0;
}
/*----------------------------------------------------------------------------*/
static void array_dst_basic (void **state)
{
  array_dst_context* c = (array_dst_context*) *state;
  for (uword i = 0; i < arr_elems (c->entries) - 1; ++i) {
    malc_log_strings s = MALC_LOG_STRS_INITIALIZER ("1", "2", "3");
    bl_err err = malc_array_dst_tbl.write ((void*) c->ad, 0, 0, &s);
    assert_int_equal (bl_ok, err.bl);
    assert_int_equal (i + 1, malc_array_dst_size (c->ad));
    char const* e = malc_array_dst_get_entry (c->ad, i);
    assert_non_null (e);
    assert_int_equal (3, strlen (e));
    assert_int_equal (0, memcmp (e, "123", 3));
  }
}
/*----------------------------------------------------------------------------*/
static void array_dst_rotation (void **state)
{
  array_dst_context* c = (array_dst_context*) *state;
  for (uword i = 0; i < arr_elems (c->entries); ++i) {
    char ch = (char) i + 1;
    malc_log_strings s = MALC_LOG_STRS_INITIALIZER ("", "", "");
    s.tstamp     = &ch;
    s.tstamp_len = 1;
    bl_err err = malc_array_dst_tbl.write ((void*) c->ad, 0, 0, &s);
    assert_int_equal (bl_ok, err.bl);
  }
  char const* e = malc_array_dst_get_entry (c->ad, 0);
  assert_int_equal (1, strlen (e));
  assert_int_equal ((char) 2, e[0]); /* was rotated */

  e = malc_array_dst_get_entry (c->ad, malc_array_dst_size (c->ad) - 1);
  assert_int_equal (1, strlen (e));
  assert_int_equal ((char) arr_elems (c->entries), e[0]); /* wasn't rotated */
}
/*----------------------------------------------------------------------------*/
static void array_dst_all_truncations (void **state)
{
  array_dst_context* c = (array_dst_context*) *state;

  /*truncation on tstamp*/
  malc_log_strings s = MALC_LOG_STRS_INITIALIZER ("1234567890", "", "");
  bl_err err = malc_array_dst_tbl.write ((void*) c->ad, 0, 0, &s);
  assert_int_equal (bl_ok, err.bl);
  char const* e = malc_array_dst_get_entry (c->ad, 0);
  assert_non_null (e);
  assert_int_equal (entry_len, strlen (e));
  assert_int_equal (0, memcmp (e, "123456789012345", entry_len));

  /*truncation on sev*/
  malc_log_strings s2 = MALC_LOG_STRS_INITIALIZER ("", "1234567890", "");
  err = malc_array_dst_tbl.write((void*) c->ad, 0, 0, &s2);
  assert_int_equal (bl_ok, err.bl);
  e = malc_array_dst_get_entry (c->ad, 1);
  assert_non_null (e);
  assert_int_equal (entry_len, strlen (e));
  assert_int_equal (0, memcmp (e, "123456789012345", entry_len));

  /*truncation on text*/
  malc_log_strings s3 = MALC_LOG_STRS_INITIALIZER ("", "", "1234567890");
  err = malc_array_dst_tbl.write((void*) c->ad, 0, 0, &s3);
  assert_int_equal (bl_ok, err.bl);
  e = malc_array_dst_get_entry (c->ad, 2);
  assert_non_null (e);
  assert_int_equal (entry_len, strlen (e));
  assert_int_equal (0, memcmp (e, "123456789012345", entry_len));
}
/*----------------------------------------------------------------------------*/
static const struct CMUnitTest tests[] = {
  cmocka_unit_test_setup (array_dst_basic, array_dst_test_setup),
  cmocka_unit_test_setup (array_dst_rotation, array_dst_test_setup),
  cmocka_unit_test_setup (array_dst_all_truncations, array_dst_test_setup),
};
/*----------------------------------------------------------------------------*/
int array_dst_tests (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*----------------------------------------------------------------------------*/
