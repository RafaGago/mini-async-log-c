#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <malc/destinations/file.h>

#include <bl/cmocka_pre.h>

#include <bl/base/error.h>
#include <bl/base/integer.h>
#include <bl/base/utility.h>
#include <bl/base/default_allocator.h>

#define FILE_PREFIX "malc_log_file_dst_test_out"
/*----------------------------------------------------------------------------*/
typedef struct file_dst_context {
  bl_u64         instance_buff[64];
  malc_file_dst* fd;
  bl_alloc_tbl   alloc;
}
file_dst_context;
/*----------------------------------------------------------------------------*/
#define MALC_LOG_STRS_INITIALIZER(t, s, txt)\
  { t, bl_lit_len (t), s, bl_lit_len (s), txt, bl_lit_len (txt) }

static void remove_log_files (void)
{
#ifndef BL_WINDOWS
  system ("rm -f "FILE_PREFIX"* > /dev/null 2>&1");
#else
  system ("del "FILE_PREFIX"*");
#endif
}
/*----------------------------------------------------------------------------*/
static int file_dst_test_setup (void **state)
{
  static file_dst_context c;
  assert_true (sizeof c.instance_buff >= malc_file_dst_tbl.size_of);
  remove_log_files();
  c.fd       = (malc_file_dst*) c.instance_buff;
  c.alloc    = bl_get_default_alloc();
  bl_err err = malc_file_dst_tbl.init ((void*) c.fd, &c.alloc);
  assert_int_equal (bl_ok, err.bl);
  *state = (void*) &c;
  return 0;
}
/*----------------------------------------------------------------------------*/
static int file_dst_test_teardown (void **state)
{
  file_dst_context* c = (file_dst_context*) *state;
  malc_file_dst_tbl.terminate ((void*) c->fd);
  remove_log_files();
  return 0;
}
/*----------------------------------------------------------------------------*/
static void cmp_file_content (char const* fname, char const* content)
{
  char rbuff[128];
  FILE* f = fopen (fname, "r");
  assert_non_null (f);
  memset (rbuff, 0, sizeof rbuff);
  fread (rbuff, sizeof (char), sizeof rbuff, f);
  fclose (f);
  assert_string_equal (rbuff, content);
}
/*----------------------------------------------------------------------------*/
static void file_dst_basic (void **state)
{
  file_dst_context* c = (file_dst_context*) *state;

  malc_file_cfg cfg;
  cfg.prefix          = FILE_PREFIX;
  cfg.suffix          = nullptr;
  cfg.max_file_size   = 0;
  cfg.max_log_files   = 0;
  cfg.time_based_name = false;
  cfg.can_remove_old_data_on_full_disk = true;
  bl_err err = malc_file_set_cfg (c->fd, &cfg);
  assert_int_equal (err.bl, bl_ok);

  malc_log_strings s = MALC_LOG_STRS_INITIALIZER ("1", "2", "3");
  err = malc_file_dst_tbl.write ((void*) c->fd, 0, 0, &s);
  assert_int_equal (err.bl, bl_ok);
  malc_file_dst_tbl.terminate ((void*) c->fd); /* force file creation*/

  cmp_file_content (FILE_PREFIX"_0", "123\n");
}
/*----------------------------------------------------------------------------*/
static void file_dst_segments (void **state)
{
  file_dst_context* c = (file_dst_context*) *state;

  malc_file_cfg cfg;
  cfg.prefix          = FILE_PREFIX;
  cfg.suffix          = nullptr;
  cfg.max_file_size   = 1;
  cfg.max_log_files   = 0;
  cfg.time_based_name = false;
  cfg.can_remove_old_data_on_full_disk = true;
  bl_err err = malc_file_set_cfg (c->fd, &cfg);
  assert_int_equal (err.bl, bl_ok);

  for (bl_uword i = 0; i < 3; ++i) {
    malc_log_strings s = MALC_LOG_STRS_INITIALIZER ("1", "2", "3");
    err = malc_file_dst_tbl.write ((void*) c->fd, 0, 0, &s);
    assert_int_equal (err.bl, bl_ok);
  }
  malc_file_dst_tbl.terminate ((void*) c->fd); /* force file creation*/

  cmp_file_content (FILE_PREFIX"_0", "123\n");
  cmp_file_content (FILE_PREFIX"_1", "123\n");
  cmp_file_content (FILE_PREFIX"_2", "123\n");
}
/*----------------------------------------------------------------------------*/
static void file_dst_rotation (void **state)
{
  file_dst_context* c = (file_dst_context*) *state;

  malc_file_cfg cfg;
  cfg.prefix          = FILE_PREFIX;
  cfg.suffix          = nullptr;
  cfg.max_file_size   = 1;
  cfg.max_log_files   = 2;
  cfg.time_based_name = false;
  cfg.can_remove_old_data_on_full_disk = true;
  bl_err err = malc_file_set_cfg (c->fd, &cfg);
  assert_int_equal (err.bl, bl_ok);

  for (bl_uword i = 0; i < 3; ++i) {
    malc_log_strings s = MALC_LOG_STRS_INITIALIZER ("1", "2", "3");
    err = malc_file_dst_tbl.write ((void*) c->fd, 0, 0, &s);
    assert_int_equal (err.bl, bl_ok);
  }
  malc_file_dst_tbl.terminate ((void*) c->fd); /* force file creation*/

  FILE* f = fopen (FILE_PREFIX"_0", "r");
  assert_null (f);
  cmp_file_content (FILE_PREFIX"_1", "123\n");
  cmp_file_content (FILE_PREFIX"_2", "123\n");
}
/*----------------------------------------------------------------------------*/
static const struct CMUnitTest tests[] = {
  cmocka_unit_test_setup_teardown(
    file_dst_basic, file_dst_test_setup, file_dst_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    file_dst_segments, file_dst_test_setup, file_dst_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    file_dst_rotation, file_dst_test_setup, file_dst_test_teardown
    ),
};
/*----------------------------------------------------------------------------*/
int file_dst_tests (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*----------------------------------------------------------------------------*/
