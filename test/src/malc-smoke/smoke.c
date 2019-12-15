#include <bl/cmocka_pre.h>
#include <bl/base/default_allocator.h>
#include <bl/base/utility.h>
#include <bl/base/thread.h>

#include <malc/malc.h>
#include <malc/destinations/array.h>

/*----------------------------------------------------------------------------*/
typedef struct context {
  bl_alloc_tbl    alloc;
  malc*           l;
  malc_array_dst* dst;
  size_t          dst_id;
  char            lines[64][1024];
}
context;
/*----------------------------------------------------------------------------*/
static context smoke_context;
/*----------------------------------------------------------------------------*/
static inline malc* get_malc_instance()
{
  return smoke_context.l;
}
/*----------------------------------------------------------------------------*/
static int setup (void **state)
{
  *state  = (void*) &smoke_context;
  context* c = (context*) &smoke_context;
  memset (c, 0, sizeof *c);
  c->alloc = bl_get_default_alloc();
  c->l     = (malc*) bl_alloc (&c->alloc,  malc_get_size());
  if (!c->l) {
    return 1;
  }
  bl_err err = malc_create (c->l, &c->alloc);
  if (err.own) {
    return err.own;
  }
  err = malc_add_destination (c->l, &c->dst_id, &malc_array_dst_tbl);
  if (err.own) {
    return err.own;
  }
  err = malc_get_destination_instance (c->l, (void**) &c->dst, c->dst_id);
  if (err.own) {
    return err.own;
  }
  malc_array_dst_set_array(
    c->dst, (char*) c->lines, bl_arr_elems (c->lines), bl_arr_elems (c->lines[0])
    );

  malc_dst_cfg dcfg;
  dcfg.log_rate_filter_time_ns = 0;
  dcfg.show_timestamp     = false;
  dcfg.show_severity      = false;
  dcfg.severity           = malc_sev_debug;
  dcfg.severity_file_path = nullptr;

  err = malc_set_destination_cfg (c->l, &dcfg, c->dst_id);
  return err.own;
}
/*----------------------------------------------------------------------------*/
static void termination_check (context* c)
{
  bl_err err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err.own, bl_nothing_to_do); /* test left work to do...*/
  err = malc_terminate (c->l, true);
  assert_int_equal (err.own, bl_ok);
  err = malc_run_consume_task (c->l, 10000);
  assert_true (err.own == bl_ok || err.own == bl_nothing_to_do);
  err = malc_run_consume_task (c->l, 10000);
  assert_true (err.own == bl_preconditions);
}
/*----------------------------------------------------------------------------*/
static int teardown (void **state)
{
  context* c = (context*) *state;
  bl_err err = malc_destroy (c->l);
  if (err.own == bl_preconditions) {
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
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread  = false;

  err = malc_init (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void all_allocation_sizes_up_to_slot_size_impl(
  void **state, bool producer_timestamp
  )
{
  /* notice that as there are internal structures prepended on each slot, this
  test is guaranteed to test all the transition sizes between a slot and two.*/
  context* c = (context*) *state;
  char send_data[sizeof c->lines[0]];

  /* making the entries on the array match the exact passed data by turning-off
  the timestamp and severity printouts */
  malc_dst_cfg dcfg;
  dcfg.log_rate_filter_time_ns = 0;
  dcfg.show_timestamp     = false;
  dcfg.show_severity      = false;
  dcfg.severity           = malc_sev_debug;
  dcfg.severity_file_path = nullptr;

  bl_err err = malc_set_destination_cfg (c->l, &dcfg, c->dst_id);
  assert_int_equal (err.own, bl_ok);

  malc_cfg cfg;
  err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  assert_true (cfg.alloc.slot_size < sizeof send_data);
  cfg.producer.timestamp = producer_timestamp;
  cfg.consumer.start_own_thread = false;

  err = malc_init (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  for (bl_uword i = 0; i < cfg.alloc.slot_size; ++i) {
    send_data[i] = '0' + (char)(i % 10);
    bl_uword datasz = i + 1;

    err = log_warning ("{}", logstrcpy (send_data, (bl_u16) datasz));
    assert_int_equal (err.own, bl_ok);

    err = malc_run_consume_task (c->l, 10000);
    assert_int_equal (err.own, bl_ok);

    bl_uword arrsz = malc_array_dst_size (c->dst);
    if (datasz > arrsz) {
      assert_int_equal (arrsz, malc_array_dst_capacity (c->dst));
    }
    else {
      assert_int_equal (arrsz, datasz);
    }
    assert_memory_equal(
      malc_array_dst_get_entry (c->dst, arrsz -1), send_data, datasz
      );
  }
  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void all_allocation_sizes_up_to_slot_size_no_tstamp (void **state)
{
  all_allocation_sizes_up_to_slot_size_impl (state, false);
}
/*----------------------------------------------------------------------------*/
static void all_allocation_sizes_up_to_slot_size_with_tstamp (void **state)
{
  all_allocation_sizes_up_to_slot_size_impl (state, true);
}
/*----------------------------------------------------------------------------*/
static void tls_allocation (void **state)
{
  static const bl_uword tls_size = 32;

  context* c = (context*) *state;
  malc_cfg cfg;
  bl_err err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  err = malc_producer_thread_local_init (c->l, tls_size);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread   = false;
  cfg.alloc.fixed_allocator_bytes = 0; /* No bounded queue */
  cfg.alloc.msg_allocator = nullptr; /* No dynamic allocation */

  err = malc_init (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  err = log_warning ("msg1: {}", 1);
  assert_int_equal (err.own, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (malc_array_dst_size (c->dst), 1);
  assert_string_equal (malc_array_dst_get_entry (c->dst, 0), "msg1: 1");

  err = log_warning ("msg2: {}", logmemcpy ((void*) &err, tls_size * 8));
  assert_int_equal (err.own, bl_alloc);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void bounded_allocation (void **state)
{
  static const bl_uword bounded_size = 128;

  context* c = (context*) *state;
  malc_cfg cfg;
  bl_err err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;
  cfg.alloc.fixed_allocator_bytes = bounded_size; /* bounded queue */
  cfg.alloc.fixed_allocator_max_slots = 1;
  cfg.alloc.fixed_allocator_per_cpu = true;
  cfg.alloc.msg_allocator = nullptr; /* No dynamic allocation */

  err = malc_init (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  err = log_warning ("msg1: {}", 1);
  assert_int_equal (err.own, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (malc_array_dst_size (c->dst), 1);
  assert_string_equal (malc_array_dst_get_entry (c->dst, 0), "msg1: 1");

  err = log_warning ("msg2: {}", logmemcpy ((void*) &err, bounded_size * 8));
  assert_int_equal (err.own, bl_alloc);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void dynamic_allocation (void **state)
{
  context* c = (context*) *state;
  malc_cfg cfg;
  bl_err err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread   = false;
  cfg.alloc.fixed_allocator_bytes = 0; /* No TLS, No bounded queue */
  assert_non_null (cfg.alloc.msg_allocator);

  err = malc_init (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  err = log_warning ("msg1: {}", 1);
  assert_int_equal (err.own, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err.own, bl_ok);

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
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread   = true;
  cfg.alloc.fixed_allocator_bytes = 0; /* No TLS, No bounded queue */
  assert_non_null (cfg.alloc.msg_allocator);

  err = malc_init (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  err = log_warning ("msg1: {}", 1);
  assert_int_equal (err.own, bl_ok);

  /* flush, so the entry is processed by the consumer thread */
  err = malc_flush (c->l);
  assert_int_equal (err.own, bl_ok);

  /* this isn't actually thread-safe, it's just the yield that makes it
  succeed */
  bl_thread_yield();
  assert_int_equal (malc_array_dst_size (c->dst), 1);
  assert_string_equal (malc_array_dst_get_entry (c->dst, 0), "msg1: 1");

  err = malc_terminate (c->l, false);
  assert_int_equal (err.own, bl_ok);
}
/*----------------------------------------------------------------------------*/
static void severity_change (void **state)
{
  context* c = (context*) *state;
  malc_cfg cfg;
  bl_err err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  malc_dst_cfg dcfg;
  dcfg.log_rate_filter_time_ns = 0;
  dcfg.show_timestamp     = false;
  dcfg.show_severity      = false;
  dcfg.severity           = malc_sev_debug;
  dcfg.severity_file_path = nullptr;

  err = malc_set_destination_cfg (c->l, &dcfg, c->dst_id);
  assert_int_equal (err.own, bl_ok);

  err = malc_init (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  err = log_debug ("unfiltered");
  assert_int_equal (err.own, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (malc_array_dst_size (c->dst), 1);
  assert_string_equal (malc_array_dst_get_entry (c->dst, 0), "unfiltered");

  dcfg.severity = malc_sev_warning;
  err = malc_set_destination_cfg (c->l, &dcfg, c->dst_id);
  assert_int_equal (err.own, bl_ok);

  err = log_debug ("filtered");
  assert_int_equal (err.own, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err.own, bl_nothing_to_do); /*filtered out at the call site*/

  assert_int_equal (malc_array_dst_size (c->dst), 1);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void severity_two_destinations (void **state)
{
  char lines[64][80];
  size_t          dst_id2;
  malc_array_dst* dst2;

  context* c = (context*) *state;

  bl_err err = malc_add_destination (c->l, &dst_id2, &malc_array_dst_tbl);
  assert_int_equal (err.own, bl_ok);
  /* the instance pointers can only be retrieved after addind the last one,
   otherwise they may be relocated. */
  err = malc_get_destination_instance (c->l, (void**) &c->dst, c->dst_id);
  assert_int_equal (err.own, bl_ok);

  err = malc_get_destination_instance (c->l, (void**) &dst2, dst_id2);
  assert_int_equal (err.own, bl_ok);
  malc_array_dst_set_array(
    dst2, (char*) lines, bl_arr_elems (lines), bl_arr_elems (lines[0])
    );

  malc_dst_cfg dcfg;
  dcfg.log_rate_filter_time_ns = 0;
  dcfg.show_timestamp     = false;
  dcfg.show_severity      = false;
  dcfg.severity           = malc_sev_debug;
  dcfg.severity_file_path = nullptr;

  err = malc_set_destination_cfg (c->l, &dcfg, c->dst_id);
  assert_int_equal (err.own, bl_ok);

  dcfg.severity = malc_sev_error;
  err = malc_set_destination_cfg (c->l, &dcfg, dst_id2);
  assert_int_equal (err.own, bl_ok);

  malc_cfg cfg;
  err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = malc_init (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  err = log_debug ("filtered");
  assert_int_equal (err.own, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (malc_array_dst_size (c->dst), 1);
  assert_string_equal (malc_array_dst_get_entry (c->dst, 0), "filtered");

  assert_int_equal (malc_array_dst_size (dst2), 0);

  err = log_error ("unfiltered");
  assert_int_equal (err.own, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err.own, bl_ok);

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
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = malc_init (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  err = log_warning(
    "{} {} {} {} {} {} {} {}",
    (bl_u8) 1,
    (bl_i8) -1,
    (bl_u16) 1,
    (bl_i16) -1,
    (bl_u32) 1,
    (bl_i32) -1,
    (bl_u64) 1,
    (bl_i64) -1
    );
  assert_int_equal (err.own, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (malc_array_dst_size (c->dst), 1);
  assert_string_equal(
    malc_array_dst_get_entry (c->dst, 0), "1 -1 1 -1 1 -1 1 -1"
    );

  err = log_warning(
    "{0Nx} {0Nx} {0Nx} {0Nx}",
    (bl_u8) 0x0e,
    (bl_u16) 0x0ffe,
    (bl_u32) 0x0ffffffe,
    (bl_u64) 0x0ffffffffffffffe
    );
  assert_int_equal (err.own, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (malc_array_dst_size (c->dst), 2);
  assert_string_equal(
    malc_array_dst_get_entry (c->dst, 1), "0e 0ffe 0ffffffe 0ffffffffffffffe"
    );

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
typedef struct smoke_refdtor_ctx {
  void* ptrs[8];
  bl_uword ptrs_count;
}
smoke_refdtor_ctx;
/*----------------------------------------------------------------------------*/
static void smoke_refdtor(void* context, malc_ref const* refs, bl_uword refs_count)
{
  smoke_refdtor_ctx* c = (smoke_refdtor_ctx*) context;
  for (bl_uword i = 0; i < refs_count; ++i) {
    c->ptrs[i] = (void*) refs[i].ref;
    free (c->ptrs[i]);
  }
  c->ptrs_count = refs_count;
}
/*----------------------------------------------------------------------------*/
static void dynargs_are_deallocated (void **state)
{
  context* c = (context*) *state;
  malc_cfg cfg;
  bl_err err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = malc_init (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  smoke_refdtor_ctx dealloc;
  dealloc.ptrs_count = 0;
  char stringv[] = "paco";
  bl_u8* v1 = (bl_u8*) malloc (sizeof stringv);
  bl_u8* v2 = (bl_u8*) malloc (sizeof stringv);
  memcpy(v1, stringv, sizeof stringv);
  memcpy(v2, stringv, sizeof stringv);

  err = log_warning(
    "streams from malloc: {} {}",
    logstrref ((char*) v1, sizeof stringv - 1),
    logmemref (v2, sizeof stringv),
    logrefdtor (smoke_refdtor, &dealloc)
    );

  assert_int_equal (err.own, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (malc_array_dst_size (c->dst), 1);
  assert_int_equal (dealloc.ptrs_count, 2);
  assert_ptr_equal (dealloc.ptrs[0], v1);
  assert_ptr_equal (dealloc.ptrs[1], v2);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void dynargs_are_deallocated_for_filtered_out_severities (void **state)
{
  /*Testing that the deallocation happens on the caller thread.*/
  context* c = (context*) *state;

  malc_dst_cfg dcfg;
  dcfg.log_rate_filter_time_ns = 0;
  dcfg.show_timestamp     = false;
  dcfg.show_severity      = false;
  dcfg.severity           = malc_sev_error;
  dcfg.severity_file_path = nullptr;

  bl_err err = malc_set_destination_cfg (c->l, &dcfg, c->dst_id);
  assert_int_equal (err.own, bl_ok);

  malc_cfg cfg;
  err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = malc_init (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  smoke_refdtor_ctx dealloc;
  dealloc.ptrs_count = 0;
  char stringv[] = "paco";
  bl_u8* v1 = (bl_u8*) malloc (sizeof stringv);
  bl_u8* v2 = (bl_u8*) malloc (sizeof stringv);
  memcpy(v1, stringv, sizeof stringv);
  memcpy(v2, stringv, sizeof stringv);

  err = log_warning(
    "streams from malloc: {} {}",
    logstrref ((char*) v1, sizeof stringv - 1),
    logmemref (v2, sizeof stringv),
    logrefdtor (smoke_refdtor, &dealloc)
    );

  assert_int_equal (err.own, bl_ok);
  assert_int_equal (dealloc.ptrs_count, 2);
  assert_ptr_equal (dealloc.ptrs[0], v1);
  assert_ptr_equal (dealloc.ptrs[1], v2);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void lazy_argument_logging (void **state)
{
  context* c = (context*) *state;

  malc_dst_cfg dcfg;
  dcfg.log_rate_filter_time_ns = 0;
  dcfg.show_timestamp     = false;
  dcfg.show_severity      = false;
  dcfg.severity           = malc_sev_error;
  dcfg.severity_file_path = nullptr;

  bl_err err = malc_set_destination_cfg (c->l, &dcfg, c->dst_id);
  assert_int_equal (err.own, bl_ok);

  malc_cfg cfg;
  err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = malc_init (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  int side_effect = 0;
  err = log_debug ("filtered out, no side effects: {}", ++side_effect);

  assert_int_equal (err.own, bl_ok);
  assert_int_equal (side_effect, 0);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void volatile_variable_logging (void **state)
{
  context* c = (context*) *state;

  malc_dst_cfg dcfg;
  dcfg.log_rate_filter_time_ns = 0;
  dcfg.show_timestamp     = false;
  dcfg.show_severity      = false;
  dcfg.severity           = malc_sev_error;
  dcfg.severity_file_path = nullptr;

  bl_err err = malc_set_destination_cfg (c->l, &dcfg, c->dst_id);
  assert_int_equal (err.own, bl_ok);

  malc_cfg cfg;
  err = malc_get_cfg (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = malc_init (c->l, &cfg);
  assert_int_equal (err.own, bl_ok);

  volatile int vol = 0;
  err = log_error ("volatile: {}", vol);
  assert_int_equal (err.own, bl_ok);

  err = malc_run_consume_task (c->l, 10000);
  assert_int_equal (err.own, bl_ok);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static const struct CMUnitTest tests[] = {
  cmocka_unit_test_setup_teardown (init_terminate, setup, teardown),
  cmocka_unit_test_setup_teardown (tls_allocation, setup, teardown),
  cmocka_unit_test_setup_teardown (bounded_allocation, setup, teardown),
  cmocka_unit_test_setup_teardown (dynamic_allocation, setup, teardown),
  cmocka_unit_test_setup_teardown (own_thread_and_flush, setup, teardown),
  cmocka_unit_test_setup_teardown(
    all_allocation_sizes_up_to_slot_size_no_tstamp, setup, teardown
    ),
  cmocka_unit_test_setup_teardown(
    all_allocation_sizes_up_to_slot_size_with_tstamp, setup, teardown
    ),
  cmocka_unit_test_setup_teardown (severity_change, setup, teardown),
  cmocka_unit_test_setup_teardown (severity_two_destinations, setup, teardown),
  cmocka_unit_test_setup_teardown (integer_formats, setup, teardown),
  cmocka_unit_test_setup_teardown (dynargs_are_deallocated, setup, teardown),
  cmocka_unit_test_setup_teardown(
    dynargs_are_deallocated_for_filtered_out_severities, setup, teardown
    ),
  cmocka_unit_test_setup_teardown (lazy_argument_logging, setup, teardown),
  cmocka_unit_test_setup_teardown (volatile_variable_logging, setup, teardown),
};
/*----------------------------------------------------------------------------*/
int main (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*----------------------------------------------------------------------------*/
