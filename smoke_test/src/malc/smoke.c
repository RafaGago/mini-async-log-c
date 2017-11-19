#include <bl/cmocka_pre.h>
#include <bl/base/default_allocator.h>
#include <bl/base/utility.h>
#include <bl/base/thread.h>

#include <malc/malc.h>
#include <malc/destinations/array.h>

/*----------------------------------------------------------------------------*/
typedef struct context {
  alloc_tbl       alloc;
  malc*           l;
  malc_array_dst* dst;
  u32             dst_id;
  char            lines[64][80];
}
context;
/*----------------------------------------------------------------------------*/
static context smoke_context;
/*----------------------------------------------------------------------------*/
static inline malc* get_malc_logger_instance()
{
  return smoke_context.l;
}
/*----------------------------------------------------------------------------*/
static int setup (void **state)
{
  *state  = (void*) &smoke_context;
  context* c = (context*) &smoke_context;
  memset (c, 0, sizeof *c);
  c->alloc = get_default_alloc();
  c->l     = (malc*) bl_alloc (&c->alloc,  malc_get_size());
  if (!c->l) {
    return 1;
  }
  bl_err err = malc_create (c->l, &c->alloc);
  if (err) {
    return err;
  }
  err = malc_add_destination (c->l, &c->dst_id, &malc_array_dst_tbl);
  if (err) {
    return err;
  }
  err = malc_get_destination_instance (c->l, (void**) &c->dst, c->dst_id);
  if (err) {
    return err;
  }
  malc_array_dst_set_array(
    c->dst, (char*) c->lines, arr_elems (c->lines), arr_elems (c->lines[0])
    );

  malc_dst_cfg dcfg;
  dcfg.log_rate_filter_time = 0;
  dcfg.show_timestamp       = false;
  dcfg.show_severity        = false;
  dcfg.severity             = malc_sev_debug;
  dcfg.severity_file_path   = nullptr;

  err = malc_set_destination_cfg (c->l, &dcfg, c->dst_id);
  return err;
}
/*----------------------------------------------------------------------------*/
static void termination_check (context* c)
{
  bl_err err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err, bl_nothing_to_do); /* test left work to do...*/
  err = malc_terminate (c->l, true);
  assert_int_equal (err, bl_ok);
  err = malc_run_consume_task (c->l, 10000);
  assert_true (err == bl_ok || err == bl_nothing_to_do);
  err = malc_run_consume_task (c->l, 10000);
  assert_true (err == bl_preconditions);
}
/*----------------------------------------------------------------------------*/
static int teardown (void **state)
{
  context* c = (context*) *state;
  bl_err err = malc_destroy (c->l);
  if (err == bl_preconditions) {
    (void) malc_terminate (c->l, true);
    (void) malc_run_consume_task (c->l, 10000);
    (void) malc_destroy (c->l);
  }
  bl_dealloc (&c->alloc, c->l);
  return 0;
}
/*----------------------------------------------------------------------------*/
static void init_terminate (void **state)
{
  context* c = (context*) *state;
  malc_cfg cfg;
  bl_err err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err, bl_ok);

  cfg.consumer.start_own_thread   = false;

  err = malc_init (c->l, &cfg);
  assert_int_equal (err, bl_ok);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void tls_allocation (void **state)
{
  static const uword tls_size = 32;

  context* c = (context*) *state;
  malc_cfg cfg;
  bl_err err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err, bl_ok);

  err = malc_producer_thread_local_init (c->l, tls_size);
  assert_int_equal (err, bl_ok);

  cfg.consumer.start_own_thread   = false;
  cfg.alloc.fixed_allocator_bytes = 0; /* No bounded queue */
  cfg.alloc.msg_allocator = nullptr; /* No dynamic allocation */

  err = malc_init (c->l, &cfg);
  assert_int_equal (err, bl_ok);

  log_warning (err, "msg1: {}", 1);
  assert_int_equal (err, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err, bl_ok);

  assert_int_equal (malc_array_dst_size (c->dst), 1);
  assert_string_equal (malc_array_dst_get_entry (c->dst, 0), "msg1: 1");

  log_warning (err, "msg2: {}", logmemcpy ((void*) &err, tls_size * 8));
  assert_int_equal (err, bl_alloc);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void bounded_allocation (void **state)
{
  static const uword bounded_size = 128;

  context* c = (context*) *state;
  malc_cfg cfg;
  bl_err err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err, bl_ok);

  cfg.consumer.start_own_thread = false;
  cfg.alloc.fixed_allocator_bytes = bounded_size; /* bounded queue */
  cfg.alloc.fixed_allocator_max_slots = 1;
  cfg.alloc.fixed_allocator_per_cpu = true;
  cfg.alloc.msg_allocator = nullptr; /* No dynamic allocation */

  err = malc_init (c->l, &cfg);
  assert_int_equal (err, bl_ok);

  log_warning (err, "msg1: {}", 1);
  assert_int_equal (err, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err, bl_ok);

  assert_int_equal (malc_array_dst_size (c->dst), 1);
  assert_string_equal (malc_array_dst_get_entry (c->dst, 0), "msg1: 1");

  log_warning (err, "msg2: {}", logmemcpy ((void*) &err, bounded_size * 8));
  assert_int_equal (err, bl_alloc);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void dynamic_allocation (void **state)
{
  context* c = (context*) *state;
  malc_cfg cfg;
  bl_err err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err, bl_ok);

  cfg.consumer.start_own_thread   = false;
  cfg.alloc.fixed_allocator_bytes = 0; /* No TLS, No bounded queue */
  assert_non_null (cfg.alloc.msg_allocator);

  err = malc_init (c->l, &cfg);
  assert_int_equal (err, bl_ok);

  log_warning (err, "msg1: {}", 1);
  assert_int_equal (err, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err, bl_ok);

  assert_int_equal (malc_array_dst_size (c->dst), 1);
  assert_string_equal (malc_array_dst_get_entry (c->dst, 0), "msg1: 1");

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void own_thread_and_flush (void **state)
{
  context* c = (context*) *state;
  malc_cfg cfg;
  bl_err err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err, bl_ok);

  cfg.consumer.start_own_thread   = true;
  cfg.alloc.fixed_allocator_bytes = 0; /* No TLS, No bounded queue */
  assert_non_null (cfg.alloc.msg_allocator);

  err = malc_init (c->l, &cfg);
  assert_int_equal (err, bl_ok);

  log_warning (err, "msg1: {}", 1);
  assert_int_equal (err, bl_ok);

  /* flush, so the entry is processed by the consumer thread */
  err = malc_flush (c->l);
  assert_int_equal (err, bl_ok);

  /* this isn't actually thread-safe, it's just the yield that makes it
  succeed */
  bl_thread_yield();
  assert_int_equal (malc_array_dst_size (c->dst), 1);
  assert_string_equal (malc_array_dst_get_entry (c->dst, 0), "msg1: 1");

  err = malc_terminate (c->l, false);
  assert_int_equal (err, bl_ok);
}
/*----------------------------------------------------------------------------*/
static void severity_change (void **state)
{
  context* c = (context*) *state;
  malc_cfg cfg;
  bl_err err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err, bl_ok);

  cfg.consumer.start_own_thread = false;

  malc_dst_cfg dcfg;
  dcfg.log_rate_filter_time = 0;
  dcfg.show_timestamp       = false;
  dcfg.show_severity        = false;
  dcfg.severity             = malc_sev_debug;
  dcfg.severity_file_path   = nullptr;

  err = malc_set_destination_cfg (c->l, &dcfg, c->dst_id);
  assert_int_equal (err, bl_ok);

  err = malc_init (c->l, &cfg);
  assert_int_equal (err, bl_ok);

  log_debug (err, "unfiltered");
  assert_int_equal (err, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err, bl_ok);

  assert_int_equal (malc_array_dst_size (c->dst), 1);
  assert_string_equal (malc_array_dst_get_entry (c->dst, 0), "unfiltered");

  dcfg.severity = malc_sev_warning;
  err = malc_set_destination_cfg (c->l, &dcfg, c->dst_id);
  assert_int_equal (err, bl_ok);

  log_debug (err, "filtered");
  assert_int_equal (err, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err, bl_nothing_to_do); /*filtered out at the call site*/

  assert_int_equal (malc_array_dst_size (c->dst), 1);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void severity_two_destinations (void **state)
{
  char lines[64][80];
  u32             dst_id2;
  malc_array_dst* dst2;

  context* c = (context*) *state;

  bl_err err = malc_add_destination (c->l, &dst_id2, &malc_array_dst_tbl);
  assert_int_equal (err, bl_ok);
  err = malc_get_destination_instance (c->l, (void**) &dst2, dst_id2);
  assert_int_equal (err, bl_ok);
  malc_array_dst_set_array(
    dst2, (char*) lines, arr_elems (lines), arr_elems (lines[0])
    );

  malc_dst_cfg dcfg;
  dcfg.log_rate_filter_time = 0;
  dcfg.show_timestamp       = false;
  dcfg.show_severity        = false;
  dcfg.severity             = malc_sev_debug;
  dcfg.severity_file_path   = nullptr;

  err = malc_set_destination_cfg (c->l, &dcfg, c->dst_id);
  assert_int_equal (err, bl_ok);

  dcfg.severity = malc_sev_error;
  err = malc_set_destination_cfg (c->l, &dcfg, dst_id2);
  assert_int_equal (err, bl_ok);

  malc_cfg cfg;
  err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = malc_init (c->l, &cfg);
  assert_int_equal (err, bl_ok);

  log_debug (err, "filtered");
  assert_int_equal (err, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err, bl_ok);

  assert_int_equal (malc_array_dst_size (c->dst), 1);
  assert_string_equal (malc_array_dst_get_entry (c->dst, 0), "filtered");

  assert_int_equal (malc_array_dst_size (dst2), 0);

  log_error (err, "unfiltered");
  assert_int_equal (err, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err, bl_ok);

  assert_int_equal (malc_array_dst_size (c->dst), 2);
  assert_string_equal (malc_array_dst_get_entry (c->dst, 1), "unfiltered");

  assert_int_equal (malc_array_dst_size (dst2), 1);
  assert_string_equal (malc_array_dst_get_entry (dst2, 0), "unfiltered");

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void integer_formats (void **state)
{
  context* c = (context*) *state;
  malc_cfg cfg;
  bl_err err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = malc_init (c->l, &cfg);
  assert_int_equal (err, bl_ok);

  log_warning(
    err,
    "{} {} {} {} {} {} {} {}",
    (u8) 1,
    (i8) -1,
    (u16) 1,
    (i16) -1,
    (u32) 1,
    (i32) -1,
    (u64) 1,
    (i64) -1,
    );
  assert_int_equal (err, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err, bl_ok);

  assert_int_equal (malc_array_dst_size (c->dst), 1);
  assert_string_equal(
    malc_array_dst_get_entry (c->dst, 0), "1 -1 1 -1 1 -1 1 -1"
    );

  log_warning(
    err,
    "{0Nx} {0Nx} {0Nx} {0Nx}",
    (u8) 0x0e,
    (u16) 0x0ffe,
    (u32) 0x0ffffffe,
    (u64) 0x0ffffffffffffffe,
    );
  assert_int_equal (err, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err, bl_ok);

  assert_int_equal (malc_array_dst_size (c->dst), 2);
  assert_string_equal(
    malc_array_dst_get_entry (c->dst, 1), "0e 0ffe 0ffffffe 0ffffffffffffffe"
    );

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static const struct CMUnitTest tests[] = {
  cmocka_unit_test_setup_teardown (init_terminate, setup, teardown),
  cmocka_unit_test_setup_teardown (tls_allocation, setup, teardown),
  cmocka_unit_test_setup_teardown (bounded_allocation, setup, teardown),
  cmocka_unit_test_setup_teardown (dynamic_allocation, setup, teardown),
  cmocka_unit_test_setup_teardown (own_thread_and_flush, setup, teardown),
  cmocka_unit_test_setup_teardown (severity_change, setup, teardown),
  cmocka_unit_test_setup_teardown (severity_two_destinations, setup, teardown),
  cmocka_unit_test_setup_teardown (integer_formats, setup, teardown),
};
/*----------------------------------------------------------------------------*/
int main (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*----------------------------------------------------------------------------*/
