#include <bl/cmocka_pre.h>
#include <bl/base/default_allocator.h>
#include <bl/base/utility.h>

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
  c->l     = bl_alloc (&c->alloc,  malc_get_size());
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
static int teardown (void **state)
{
  bool had_err = false;
  context* c = (context*) *state;
  bl_err err = malc_run_consume_task (c->l, 50000);
  had_err |= (err != bl_nothing_to_do); /* test left work to do...*/
  err = malc_terminate (c->l, true);
  had_err |= (err != bl_ok);
  err = malc_run_consume_task (c->l, 50000);
  had_err |= (err != bl_ok && err != bl_nothing_to_do);
  err = malc_run_consume_task (c->l, 50000);
  had_err |= (err != bl_preconditions);
  malc_destroy (c->l);
  bl_dealloc (&c->alloc, c->l);
  return had_err;
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

  err = malc_run_consume_task (c->l, 50000);
  assert_int_equal (err, bl_ok);

  assert_int_equal (malc_array_dst_size (c->dst), 1);
  assert_string_equal (malc_array_dst_get_entry (c->dst, 0), "msg1: 1");
}
/*----------------------------------------------------------------------------*/
static const struct CMUnitTest tests[] = {
  cmocka_unit_test_setup_teardown (init_terminate, setup, teardown),
  cmocka_unit_test_setup_teardown (dynamic_allocation, setup, teardown),
};
/*----------------------------------------------------------------------------*/
int main (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*----------------------------------------------------------------------------*/