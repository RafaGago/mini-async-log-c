#include <string.h>
#include <stdio.h>

#include <bl/cmocka_pre.h>
#include <bl/base/default_allocator.h>
#include <bl/base/time.h>
#include <bl/time_extras/time_extras.h>

#include <malc/destinations.h>

#define SEV_FILE_NAME "destinations_test_severity.txt"

/*----------------------------------------------------------------------------*/
typedef struct mock_dest {
  bl_u8* terminate;
  bl_u32 flush;
  bl_u16 idle_task;
  bl_u64 write;
}
mock_dest;
/*----------------------------------------------------------------------------*/
static bl_err mock_dest_init (void* instance, bl_alloc_tbl const* alloc)
{
  mock_dest* d = (mock_dest*) instance;
  memset (d, 0, sizeof *d);
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static void mock_dest_terminate (void* instance)
{
  mock_dest* d = (mock_dest*) instance;
  ++d->terminate;
}
/*----------------------------------------------------------------------------*/
static bl_err mock_dest_flush (void* instance)
{
  mock_dest* d = (mock_dest*) instance;
  ++d->flush;
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static bl_err mock_dest_idle_task (void* instance)
{
  mock_dest* d = (mock_dest*) instance;
  ++d->idle_task;
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static bl_err mock_dest_write(
  void* instance, bl_timept64 now, bl_uword sev, malc_log_strings const* s
  )
{
  mock_dest* d = (mock_dest*) instance;
  ++d->write;
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static const malc_dst mock_dst_tbl = {
  sizeof (mock_dest),
  &mock_dest_init,
  &mock_dest_terminate,
  &mock_dest_flush,
  &mock_dest_idle_task,
  &mock_dest_write,
};
/*----------------------------------------------------------------------------*/
typedef struct destinations_context {
  malc_dst     tbls[2];
  bl_alloc_tbl alloc;
  destinations d;
}
destinations_context;
/*----------------------------------------------------------------------------*/
static int dsts_test_setup (void **state)
{
  static destinations_context c;
  c.tbls[0] = mock_dst_tbl;
  c.tbls[1] = mock_dst_tbl;
  c.alloc   = bl_get_default_alloc();
  destinations_init (&c.d, &c.alloc);
  *state = &c;
  return 0;
}
/*----------------------------------------------------------------------------*/
static int dsts_test_teardown (void **state)
{
  (void) remove (SEV_FILE_NAME);
  destinations_context* c = (destinations_context*) *state;
  destinations_destroy (&c->d);
  return 0;
}
/*----------------------------------------------------------------------------*/
static void destinations_do_add(
  destinations_context* c, bl_u32* ids, mock_dest** mock
  )
{
  bl_err err;
  err = destinations_add (&c->d, &ids[0], &c->tbls[0]);
  assert_int_equal (bl_ok, err.bl);
  err = destinations_add (&c->d, &ids[1], &c->tbls[1]);
  assert_int_equal (bl_ok, err.bl);

  err = destinations_get_instance (&c->d, (void*) &mock[0], ids[0]);
  assert_int_equal (bl_ok, err.bl);
  err = destinations_get_instance (&c->d, (void*) &mock[1], ids[1]);
  assert_int_equal (bl_ok, err.bl);
}
/*----------------------------------------------------------------------------*/
static void destinations_cfg_test (void **state)
{
  destinations_context* c = (destinations_context*) *state;
  bl_u32 id[2];
  mock_dest* mock[2];
  bl_err err;
  destinations_do_add (c, id, mock);

  malc_dst_cfg cfg[2];
  err = destinations_get_cfg (&c->d, &cfg[0], id[0]);
  assert_int_equal (bl_ok, err.bl);
  err = destinations_get_cfg (&c->d, &cfg[1], id[1]);
  assert_int_equal (bl_ok, err.bl);

  cfg[0].severity = malc_sev_debug;
  cfg[1].severity = malc_sev_trace;

  err = destinations_set_cfg (&c->d, &cfg[1], id[1]);
  assert_int_equal (bl_ok, err.bl);
  err = destinations_set_cfg (&c->d, &cfg[0], id[0]);
  assert_int_equal (bl_ok, err.bl);

  memset (cfg, 0, sizeof cfg);

  err = destinations_get_cfg (&c->d, &cfg[0], id[0]);
  assert_int_equal (bl_ok, err.bl);
  err = destinations_get_cfg (&c->d, &cfg[1], id[1]);
  assert_int_equal (bl_ok, err.bl);

  assert_int_equal (malc_sev_debug, cfg[0].severity);
  assert_int_equal (malc_sev_trace, cfg[1].severity);
  assert_int_equal (malc_sev_debug, destinations_min_severity (&c->d));
}
/*----------------------------------------------------------------------------*/
static void destinations_idle_task_test (void **state)
{
  destinations_context* c = (destinations_context*) *state;
  bl_u32 id[2];
  mock_dest* mock[2];

  c->tbls[0].idle_task = nullptr;
  destinations_do_add (c, id, mock);
  destinations_idle_task (&c->d, 0);

  assert_int_equal (mock[0]->idle_task, 0);
  assert_int_equal (mock[1]->idle_task, 1);
}
/*----------------------------------------------------------------------------*/
static void destinations_flush_test (void **state)
{
  destinations_context* c = (destinations_context*) *state;
  bl_u32 id[2];
  mock_dest* mock[2];
  c->tbls[0].flush = nullptr;
  destinations_do_add (c, id, mock);
  destinations_flush (&c->d);

  assert_int_equal (mock[0]->flush, 0);
  assert_int_equal (mock[1]->flush, 1);
}
/*----------------------------------------------------------------------------*/
static void destinations_write_test (void **state)
{
  destinations_context* c = (destinations_context*) *state;
  bl_u32 id[2];
  mock_dest* mock[2];

  destinations_do_add (c, id, mock);
  malc_log_strings strings;
  memset (&strings, 0, sizeof strings);
  destinations_write (&c->d, 0, 0, malc_sev_critical, &strings);

  assert_int_equal (mock[0]->write, 1);
  assert_int_equal (mock[1]->write, 1);
}
/*----------------------------------------------------------------------------*/
static void destinations_write_sev_test (void **state)
{
  destinations_context* c = (destinations_context*) *state;
  bl_u32           id[2];
  mock_dest*       mock[2];
  malc_dst_cfg     cfg;
  bl_err           err;
  malc_log_strings strings;

  memset (&strings, 0, sizeof strings);

  destinations_do_add (c, id, mock);

  err = destinations_get_cfg (&c->d, &cfg, id[0]);
  assert_int_equal (bl_ok, err.bl);
  cfg.severity = malc_sev_critical;
  err = destinations_set_cfg (&c->d, &cfg, id[0]);
  assert_int_equal (bl_ok, err.bl);

  destinations_write (&c->d, 0, 0, malc_sev_error, &strings);
  assert_int_equal (mock[0]->write, 0);
  assert_int_equal (mock[1]->write, 1);

  destinations_write (&c->d, 0, 0, malc_sev_critical, &strings);
  assert_int_equal (mock[0]->write, 1);
  assert_int_equal (mock[1]->write, 2);
}
/*----------------------------------------------------------------------------*/
static void write_sev_file (char const* text)
{
  FILE* f = fopen (SEV_FILE_NAME, "wb");
  assert_non_null (f);
  (void) fwrite (text, 1, strlen (text), f);
  int e = ferror (f);
  fclose (f);
  assert_int_equal (0, e);
}
/*----------------------------------------------------------------------------*/
static void destinations_write_rate_filter_test (void **state)
{
  destinations_context* c = (destinations_context*) *state;
  bl_u32           id[2];
  mock_dest*       mock[2];
  malc_dst_cfg     cfg;
  bl_err           err;
  malc_log_strings strings;

  memset (&strings, 0, sizeof strings);

  destinations_do_add (c, id, mock);

  err = destinations_get_cfg (&c->d, &cfg, id[0]);
  assert_int_equal (bl_ok, err.bl);
  cfg.log_rate_filter_time_ns = 2000;
  err = destinations_set_cfg (&c->d, &cfg, id[0]);
  assert_int_equal (bl_ok, err.bl);

  err = destinations_get_cfg (&c->d, &cfg, id[1]);
  assert_int_equal (bl_ok, err.bl);
  cfg.log_rate_filter_time_ns = 2000;
  err = destinations_set_cfg (&c->d, &cfg, id[1]);
  assert_int_equal (bl_ok, err.bl);

  malc_security sec;
  sec.log_rate_filter_watch_count  = 4;
  sec.log_rate_filter_min_severity = malc_sev_debug;
  err = destinations_set_rate_limit_settings (&c->d, &sec);
  assert_int_equal (bl_ok, err.bl);

  bl_timept64 t = 0;
  destinations_write (&c->d, 0, t, malc_sev_critical, &strings);
  assert_int_equal (mock[0]->write, 1);
  assert_int_equal (mock[1]->write, 1);

  t += 2000;
  destinations_write (&c->d, 0, t, malc_sev_critical, &strings);
  assert_int_equal (mock[0]->write, 2);
  assert_int_equal (mock[1]->write, 2);

  t += 1000;
  destinations_write (&c->d, 0, t, malc_sev_critical, &strings);
  /*filtered out: less than 2 us from last entry */
  assert_int_equal (mock[0]->write, 2);
  assert_int_equal (mock[1]->write, 2);

  t += 1000;
  destinations_write (&c->d, 0, t, malc_sev_critical, &strings);
  /*still filtered out: the filtered out message before updated the internal
  counter */
  assert_int_equal (mock[0]->write, 2);
  assert_int_equal (mock[1]->write, 2);

}
/*----------------------------------------------------------------------------*/
static void destinations_write_rate_filter_severity_test (void **state)
{
  destinations_context* c = (destinations_context*) *state;
  bl_u32              id[2];
  mock_dest*       mock[2];
  malc_dst_cfg     cfg;
  bl_err           err;
  malc_log_strings strings;

  memset (&strings, 0, sizeof strings);

  destinations_do_add (c, id, mock);

  err = destinations_get_cfg (&c->d, &cfg, id[0]);
  assert_int_equal (bl_ok, err.bl);
  cfg.log_rate_filter_time_ns = 2000;
  cfg.severity                = malc_sev_debug;
  err = destinations_set_cfg (&c->d, &cfg, id[0]);
  assert_int_equal (bl_ok, err.bl);

  err = destinations_get_cfg (&c->d, &cfg, id[1]);
  assert_int_equal (bl_ok, err.bl);
  cfg.log_rate_filter_time_ns = 2000;
  cfg.severity                = malc_sev_debug;
  err = destinations_set_cfg (&c->d, &cfg, id[1]);
  assert_int_equal (bl_ok, err.bl);

  malc_security sec;
  sec.log_rate_filter_watch_count  = 4;
  sec.log_rate_filter_min_severity = malc_sev_warning;
  err = destinations_set_rate_limit_settings (&c->d, &sec);
  assert_int_equal (bl_ok, err.bl);

  bl_timept64 t = 0;
  destinations_write (&c->d, 0, t, malc_sev_critical, &strings);
  assert_int_equal (mock[0]->write, 1);
  assert_int_equal (mock[1]->write, 1);

  t += 1000;
  destinations_write (&c->d, 0, t, malc_sev_warning, &strings);
  /*filtered out: severity */
  assert_int_equal (mock[0]->write, 1);
  assert_int_equal (mock[1]->write, 1);

  t += 1000;
  destinations_write (&c->d, 0, t, malc_sev_note, &strings);
  /*not filtered out: severity */
  assert_int_equal (mock[0]->write, 2);
  assert_int_equal (mock[1]->write, 2);
}
/*----------------------------------------------------------------------------*/
static void destinations_sev_file_test (void **state)
{
  destinations_context* c = (destinations_context*) *state;
  bl_u32           id[2];
  mock_dest*       mock[2];
  malc_dst_cfg     cfg;
  bl_err           err;
  malc_log_strings strings;

  memset (&strings, 0, sizeof strings);

  destinations_do_add (c, id, mock);

  err = destinations_get_cfg (&c->d, &cfg, id[0]);
  assert_int_equal (bl_ok, err.bl);

  cfg.severity_file_path = SEV_FILE_NAME;
  cfg.severity = malc_sev_debug;
  err = destinations_set_cfg (&c->d, &cfg, id[0]);
  assert_int_equal (bl_ok, err.bl);

  write_sev_file ("critical");
  destinations_idle_task (&c->d, 0);
  err = destinations_get_cfg (&c->d, &cfg, id[0]);
  assert_int_equal (cfg.severity, malc_sev_critical);

  write_sev_file ("error");
  destinations_idle_task (&c->d, 0);
  err = destinations_get_cfg (&c->d, &cfg, id[0]);
  assert_int_equal (cfg.severity, malc_sev_error);

  write_sev_file ("warning");
  destinations_idle_task (&c->d, 0);
  err = destinations_get_cfg (&c->d, &cfg, id[0]);
  assert_int_equal (cfg.severity, malc_sev_warning);

  write_sev_file ("note");
  destinations_idle_task (&c->d, 0);
  err = destinations_get_cfg (&c->d, &cfg, id[0]);
  assert_int_equal (cfg.severity, malc_sev_note);

  write_sev_file ("trace");
  destinations_idle_task (&c->d, 0);
  err = destinations_get_cfg (&c->d, &cfg, id[0]);
  assert_int_equal (cfg.severity, malc_sev_trace);

  write_sev_file ("debug");
  destinations_idle_task (&c->d, 0);
  err = destinations_get_cfg (&c->d, &cfg, id[0]);
  assert_int_equal (cfg.severity, malc_sev_debug);
}
/*----------------------------------------------------------------------------*/
static const struct CMUnitTest tests[] = {
  cmocka_unit_test_setup_teardown(
    destinations_cfg_test, dsts_test_setup, dsts_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    destinations_idle_task_test, dsts_test_setup, dsts_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    destinations_flush_test, dsts_test_setup, dsts_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    destinations_write_test, dsts_test_setup, dsts_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    destinations_write_sev_test, dsts_test_setup, dsts_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    destinations_write_rate_filter_test, dsts_test_setup, dsts_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    destinations_write_rate_filter_severity_test,
    dsts_test_setup,
    dsts_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    destinations_sev_file_test, dsts_test_setup, dsts_test_teardown
    ),
};
/*----------------------------------------------------------------------------*/
int destinations_tests (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*----------------------------------------------------------------------------*/
