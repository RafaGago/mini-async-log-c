#include <algorithm>
#include <numeric>
#include <limits>
#include <string>
#include <memory>
#include <vector>

#include <bl/base/default_allocator.h>
#include <bl/base/utility.h>
#include <bl/base/thread.h>

#include <malcpp/malcpp.hpp>
/* cmocka is so braindead to define a fail() macro!!!, which clashes with e.g.
ostream's fail(), we include this header the last and hope it never breaks.*/
#include <bl/cmocka_pre.h>

typedef malcpp::malcpp<false, false, false> malc_nonthrow;
/*----------------------------------------------------------------------------*/
typedef struct context {
  malc_nonthrow                         log;
  malcpp::dst_access<malcpp::array_dst> dst;
  bl_u32                                dst_id;
  char                                  lines[64][1024];
}
context;
/*----------------------------------------------------------------------------*/
static context smoke_context;
/*----------------------------------------------------------------------------*/
static inline malc_nonthrow& get_malc_instance()
{
  return smoke_context.log;
}
/*----------------------------------------------------------------------------*/
static int setup (void **state)
{
  *state     = (void*) &smoke_context;
  context* c = (context*) &smoke_context;
  bl_err err = c->log.construct();
  if (err.own) {
    return err.own;
  }
  c->dst = c->log.add_destination<malcpp::array_dst>();
  if (!c->dst.is_valid()) {
    return 1;
  }
  c->dst.try_get()->set_array(
    (char*) c->lines, bl_arr_elems (c->lines), bl_arr_elems (c->lines[0])
    );

  malcpp::dst_cfg dcfg;
  dcfg.log_rate_filter_time_ns = 0;
  dcfg.show_timestamp     = false;
  dcfg.show_severity      = false;
  dcfg.severity           = malcpp::sev_debug;
  dcfg.severity_file_path = nullptr;

  err = c->dst.set_cfg (dcfg);
  return err.own;
}
/*----------------------------------------------------------------------------*/
static void termination_check (context* c)
{
  bl_err err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_nothing_to_do); /* test left work to do...*/
  err = c->log.terminate (true);
  assert_int_equal (err.own, bl_ok);
  err = c->log.run_consume_task (10000);
  assert_true (err.own == bl_ok || err.own == bl_nothing_to_do);
  err = c->log.run_consume_task (10000);
  assert_true (err.own == bl_preconditions);
}
/*----------------------------------------------------------------------------*/
static int teardown (void **state)
{
  context* c = (context*) *state;
  c->log.destroy();
  return 0;
}
/*----------------------------------------------------------------------------*/
static void init_terminate (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread  = false;

  err = c->log.init (cfg);
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
  malcpp::dst_cfg dcfg;
  dcfg.log_rate_filter_time_ns = 0;
  dcfg.show_timestamp     = false;
  dcfg.show_severity      = false;
  dcfg.severity           = malcpp::sev_debug;
  dcfg.severity_file_path = nullptr;

  bl_err err = c->dst.set_cfg (dcfg);
  assert_int_equal (err.own, bl_ok);

  malcpp::cfg cfg;
  err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  assert_true (cfg.alloc.slot_size < sizeof send_data);
  cfg.producer.timestamp = producer_timestamp;
  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  for (bl_uword i = 0; i < cfg.alloc.slot_size; ++i) {
    send_data[i] = '0' + (char)(i % 10);
    bl_uword datasz = i + 1;

    err = log_warning ("{}", malcpp::strcp (send_data, (bl_u16) datasz));
    assert_int_equal (err.own, bl_ok);

    err = c->log.run_consume_task (10000);
    assert_int_equal (err.own, bl_ok);

    bl_uword arrsz = c->dst.try_get()->size();
    if (datasz > arrsz) {
      assert_int_equal (arrsz, c->dst.try_get()->capacity());
    }
    else {
      assert_int_equal (arrsz, datasz);
    }
    assert_memory_equal ((*c->dst.try_get())[arrsz -1], send_data, datasz);
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
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  err = c->log.producer_thread_local_init (tls_size);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread   = false;
  cfg.alloc.fixed_allocator_bytes = 0; /* No bounded queue */
  cfg.alloc.msg_allocator = nullptr; /* No dynamic allocation */

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  err = log_warning ("msg1: {}", 1);
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (c->dst.try_get()->size(), 1);
  assert_string_equal ((*c->dst.try_get())[0], "msg1: 1");

  err = log_warning ("msg2: {}", malcpp::memcp ((void*) &err, tls_size * 8));
  assert_int_equal (err.own, bl_alloc);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void bounded_allocation (void **state)
{
  static const bl_uword bounded_size = 128;

  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;
  cfg.alloc.fixed_allocator_bytes = bounded_size; /* bounded queue */
  cfg.alloc.fixed_allocator_max_slots = 1;
  cfg.alloc.fixed_allocator_per_cpu = true;
  cfg.alloc.msg_allocator = nullptr; /* No dynamic allocation */

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  err = log_warning ("msg1: {}", 1);
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (c->dst.try_get()->size(), 1);
  assert_string_equal ((*c->dst.try_get())[0], "msg1: 1");

  err = log_warning(
    "msg2: {}", malcpp::memcp ((void*) &err, bounded_size * 8)
    );
  assert_int_equal (err.own, bl_alloc);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void dynamic_allocation (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread   = false;
  cfg.alloc.fixed_allocator_bytes = 0; /* No TLS, No bounded queue */
  assert_non_null (cfg.alloc.msg_allocator);

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  err = log_warning ("msg1: {}", 1);
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (c->dst.try_get()->size(), 1);
  assert_string_equal ((*c->dst.try_get())[0], "msg1: 1");

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void own_thread_and_flush (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread   = true;
  cfg.alloc.fixed_allocator_bytes = 0; /* No TLS, No bounded queue */
  assert_non_null (cfg.alloc.msg_allocator);

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  err = log_warning ("msg1: {}", 1);
  assert_int_equal (err.own, bl_ok);

  /* flush, so the entry is processed by the consumer thread */
  err = c->log.flush();
  assert_int_equal (err.own, bl_ok);

  /* this isn't actually thread-safe, it's just the yield that makes it
  succeed */
  bl_thread_yield();
  assert_int_equal (c->dst.try_get()->size(), 1);
  assert_string_equal ((*c->dst.try_get())[0], "msg1: 1");

  err = c->log.terminate (false);
  assert_int_equal (err.own, bl_ok);
}
/*----------------------------------------------------------------------------*/
static void severity_change (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  malcpp::dst_cfg dcfg;
  dcfg.log_rate_filter_time_ns = 0;
  dcfg.show_timestamp     = false;
  dcfg.show_severity      = false;
  dcfg.severity           = malcpp::sev_debug;
  dcfg.severity_file_path = nullptr;

  err = c->dst.set_cfg (dcfg);
  assert_int_equal (err.own, bl_ok);

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  err = log_debug ("unfiltered");
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (c->dst.try_get()->size(), 1);
  assert_string_equal ((*c->dst.try_get())[0], "unfiltered");

  dcfg.severity = malcpp::sev_warning;
  err = c->dst.set_cfg (dcfg);
  assert_int_equal (err.own, bl_ok);

  err = log_debug ("filtered");
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_nothing_to_do); /*filtered out at the call site*/

  assert_int_equal (c->dst.try_get()->size(), 1);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void severity_two_destinations (void **state)
{
  char lines[64][80];
  context* c = (context*) *state;
  decltype (c->dst) dst2;

  dst2 = c->log.add_destination<malcpp::array_dst>();
  assert_true (dst2.is_valid());

  dst2.try_get()->set_array(
    (char*) lines, bl_arr_elems (lines), bl_arr_elems (lines[0])
    );

  malcpp::dst_cfg dcfg;
  dcfg.log_rate_filter_time_ns = 0;
  dcfg.show_timestamp     = false;
  dcfg.show_severity      = false;
  dcfg.severity           = malcpp::sev_debug;
  dcfg.severity_file_path = nullptr;

  bl_err err = c->dst.set_cfg (dcfg);
  assert_int_equal (err.own, bl_ok);

  dcfg.severity = malcpp::sev_error;
  err = dst2.set_cfg (dcfg);
  assert_int_equal (err.own, bl_ok);

  malcpp::cfg cfg;
  err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  err = log_debug ("filtered");
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (c->dst.try_get()->size(), 1);
  assert_string_equal ((*c->dst.try_get())[0], "filtered");

  assert_int_equal (dst2.try_get()->size(), 0);

  err = log_error ("unfiltered");
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (c->dst.try_get()->size(), 2);
  assert_string_equal ((*c->dst.try_get())[1], "unfiltered");

  assert_int_equal (dst2.try_get()->size(), 1);
  assert_string_equal ((*dst2.try_get())[0], "unfiltered");

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void integer_formats (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
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

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (c->dst.try_get()->size(), 1);
  assert_string_equal ((*c->dst.try_get())[0], "1 -1 1 -1 1 -1 1 -1");

  err = log_warning(
    "{.wx} {.wx} {.wx} {.wx}",
    (bl_u8) 0x0e,
    (bl_u16) 0x0ffe,
    (bl_u32) 0x0ffffffe,
    (bl_u64) 0x0ffffffffffffffe
    );
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (c->dst.try_get()->size(), 2);
  assert_string_equal(
     (*c->dst.try_get())[1], "0e 0ffe 0ffffffe 0ffffffffffffffe"
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
static void smoke_refdtor(
  void* context, malcpp::malc_ref const* refs, bl_uword refs_count
  )
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
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  smoke_refdtor_ctx dealloc;
  dealloc.ptrs_count = 0;
  char stringv[] = "paco";
  bl_u8* v1 = (bl_u8*) malloc (sizeof stringv);
  bl_u8* v2 = (bl_u8*) malloc (sizeof stringv);
  memcpy (v1, stringv, sizeof stringv);
  memcpy (v2, stringv, sizeof stringv);

  err = log_warning(
    "streams from malloc: {} {}",
    malcpp::strref ((char*) v1, sizeof stringv - 1),
    malcpp::memref (v2, sizeof stringv),
    malcpp::refdtor (smoke_refdtor, &dealloc)
    );

  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (c->dst.try_get()->size(), 1);
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

  malcpp::dst_cfg dcfg;
  dcfg.log_rate_filter_time_ns = 0;
  dcfg.show_timestamp     = false;
  dcfg.show_severity      = false;
  dcfg.severity           = malcpp::sev_error;
  dcfg.severity_file_path = nullptr;

  bl_err err = c->dst.set_cfg (dcfg);
  assert_int_equal (err.own, bl_ok);

  malcpp::cfg cfg;
  err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  smoke_refdtor_ctx dealloc;
  dealloc.ptrs_count = 0;
  char stringv[] = "paco";
  bl_u8* v1 = (bl_u8*) malloc (sizeof stringv);
  bl_u8* v2 = (bl_u8*) malloc (sizeof stringv);
  memcpy (v1, stringv, sizeof stringv);
  memcpy (v2, stringv, sizeof stringv);

  err = log_warning(
    "streams from malloc: {} {}",
    malcpp::strref ((char*) v1, sizeof stringv - 1),
    malcpp::memref (v2, sizeof stringv),
    malcpp::refdtor (smoke_refdtor, &dealloc)
    );

  assert_int_equal (err.own, bl_ok);
  assert_int_equal (dealloc.ptrs_count, 2);
  assert_ptr_equal (dealloc.ptrs[0], v1);
  assert_ptr_equal (dealloc.ptrs[1], v2);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void string_shared_ptr (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  auto ptr = std::make_shared<std::string> ("paco");
  assert_int_equal (ptr.use_count(), 1);

  err = log_warning ("{}", ptr);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (ptr.use_count(), 2);

  auto const& ptrc = ptr;
  err = log_warning ("{}", ptrc);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (ptr.use_count(), 3);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (ptr.use_count(), 1);
  assert_int_equal (c->dst.try_get()->size(), 2);
  assert_string_equal ((*c->dst.try_get())[0], "paco");
  assert_string_equal ((*c->dst.try_get())[1], "paco");

  err = log_warning ("{}", std::move (ptr));
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (ptr.use_count(), 0);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (c->dst.try_get()->size(), 3);
  assert_string_equal ((*c->dst.try_get())[2], "paco");

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void vector_shared_ptr (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  auto ptr = std::make_shared<std::vector<bl_u8> >(
    std::initializer_list<bl_u8>{ 1, 2, 3 }
    );
  assert_int_equal (ptr.use_count(), 1);

  err = log_warning ("{}", ptr);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (ptr.use_count(), 2);

  auto const& ptrc = ptr;
  err = log_warning ("{}", ptrc);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (ptr.use_count(), 3);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (ptr.use_count(), 1);
  assert_int_equal (c->dst.try_get()->size(), 2);
  assert_string_equal ((*c->dst.try_get())[0], "u08[1 2 3]");
  assert_string_equal ((*c->dst.try_get())[1], "u08[1 2 3]");

  err = log_warning ("{}", std::move (ptr));
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (ptr.use_count(), 0);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (c->dst.try_get()->size(), 3);
  assert_string_equal ((*c->dst.try_get())[2], "u08[1 2 3]");

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void vector_shared_ptr_truncation (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  using counttype =
    decltype (((malcpp::malc_obj_log_data*) nullptr)->data.builtin.count);
  const auto maxcount = std::numeric_limits<counttype>::max();

  auto ptr = std::make_shared<std::vector<bl_u8> >(maxcount + 8);
  std::iota (ptr->begin(), ptr->end(), 0);

  assert_int_equal (ptr.use_count(), 1);

  err = log_warning("{}", ptr);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (ptr.use_count(), 2);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (ptr.use_count(), 1);
  assert_int_equal (c->dst.try_get()->size(), 1);
  std::string str {(*c->dst.try_get())[0]};
  // just counting spaces, as it will log [1 2 3 4...]
  assert_int_equal (std::count (str.begin(), str.end(), ' '), maxcount - 1);
  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void string_weak_ptr (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  auto ptr = std::make_shared<std::string> ("paco");
  auto w   = std::weak_ptr<std::string> (ptr);
  assert_int_equal (ptr.use_count(), 1);

  err = log_warning ("{}", w);
  assert_int_equal (err.own, bl_ok);

  auto const& wkc = w;
  err = log_warning ("{}", wkc);
  assert_int_equal (err.own, bl_ok);

  err = log_warning ("{}", std::move (w));
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (ptr.use_count(), 1);

  err = log_warning(
    "{}", std::weak_ptr<std::string> (std::make_shared<std::string> ("paco"))
    );
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (c->dst.try_get()->size(), 4);
  assert_string_equal ((*c->dst.try_get())[0], "paco");
  assert_string_equal ((*c->dst.try_get())[1], "paco");
  assert_string_equal ((*c->dst.try_get())[2], "paco");
  assert_string_equal ((*c->dst.try_get())[3], MALC_CPP_NULL_SMART_PTR_STR);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void vector_weak_ptr (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  auto ptr = std::make_shared<std::vector<bl_u8> >(
    std::initializer_list<bl_u8>{ 1, 2, 3 }
    );
  assert_int_equal (ptr.use_count(), 1);

  auto w = std::weak_ptr<std::vector<bl_u8> > (ptr);
  err = log_warning ("{}", w);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (ptr.use_count(), 1);

  auto const& wc = w;
  err = log_warning ("{}", wc);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (ptr.use_count(), 1);

  err = log_warning ("{}", std::move (w));
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (ptr.use_count(), 1);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (ptr.use_count(), 1);
  assert_int_equal (c->dst.try_get()->size(), 3);
  assert_string_equal ((*c->dst.try_get())[0], "u08[1 2 3]");
  assert_string_equal ((*c->dst.try_get())[1], "u08[1 2 3]");
  assert_string_equal ((*c->dst.try_get())[2], "u08[1 2 3]");

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void string_unique_ptr (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  auto ptr = std::unique_ptr<std::string> (new std::string ("paco"));
  err = log_warning ("{}", std::move (ptr));
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (c->dst.try_get()->size(), 1);
  assert_string_equal ((*c->dst.try_get())[0], "paco");

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void vector_unique_ptr (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  auto ptr = std::unique_ptr<std::vector<bl_u8> >(
    new std::vector<bl_u8>{ 1, 2, 3 }
    );
  err = log_warning ("{}", std::move (ptr));
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (c->dst.try_get()->size(), 1);
  assert_string_equal ((*c->dst.try_get())[0], "u08[1 2 3]");

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
struct ostreamable_type {
  int a{1};
  int b{2};
};
std::ostream & operator << (std::ostream &out, const ostreamable_type &t)
{
  out << "ostreamable: " << t.a << ", " << t.b ;
  return out;
}
/*----------------------------------------------------------------------------*/
static void ostreamable_type_by_value (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  err = log_warning ("{}", malcpp::ostr (ostreamable_type()));
  assert_int_equal (err.own, bl_ok);

  auto v = ostreamable_type();
  err = log_warning ("{}", malcpp::ostr (v));
  assert_int_equal (err.own, bl_ok);

  err = log_warning ("{}", malcpp::ostr (3));
  assert_int_equal (err.own, bl_ok);

  int four = 4;
  err = log_warning ("{}", malcpp::ostr (four));
  assert_int_equal (err.own, bl_ok);

  const int& four_ref = 4;
  err = log_warning ("{}", malcpp::ostr (four_ref));
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (c->dst.try_get()->size(), 5);
  assert_string_equal ((*c->dst.try_get())[0], "ostreamable: 1, 2");
  assert_string_equal ((*c->dst.try_get())[1], "ostreamable: 1, 2");
  assert_string_equal ((*c->dst.try_get())[2], "3");
  assert_string_equal ((*c->dst.try_get())[3], "4");
  assert_string_equal ((*c->dst.try_get())[4], "4");

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void ostreamable_type_by_unique_ptr (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  auto u = std::unique_ptr<ostreamable_type> (new ostreamable_type());
  err = log_warning ("{}", malcpp::ostr (std::move (u)));
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (c->dst.try_get()->size(), 1);
  assert_string_equal ((*c->dst.try_get())[0], "ostreamable: 1, 2");

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void ostreamable_type_by_shared_ptr (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  auto ptr = std::shared_ptr<ostreamable_type> (new ostreamable_type());
  err = log_warning ("{}", malcpp::ostr (ptr));
  assert_int_equal (err.own, bl_ok);

  auto const& ptrc = ptr;
  err = log_warning ("{}", malcpp::ostr (ptrc));
  assert_int_equal (err.own, bl_ok);

  err = log_warning ("{}", malcpp::ostr (std::move (ptr)));
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (c->dst.try_get()->size(), 3);
  assert_string_equal ((*c->dst.try_get())[0], "ostreamable: 1, 2");
  assert_string_equal ((*c->dst.try_get())[1], "ostreamable: 1, 2");
  assert_string_equal ((*c->dst.try_get())[2], "ostreamable: 1, 2");

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void ostreamable_type_by_weak_ptr (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  std::shared_ptr<ostreamable_type> ptr (new ostreamable_type());
  auto wk = std::weak_ptr<ostreamable_type> (ptr);

  err = log_warning ("{}", malcpp::ostr (wk));
  assert_int_equal (err.own, bl_ok);

  const auto& wkc = wk;
  err = log_warning ("{}", malcpp::ostr (wkc));
  assert_int_equal (err.own, bl_ok);

  err = log_warning ("{}", malcpp::ostr (std::move (wk)));
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (c->dst.try_get()->size(), 3);
  assert_string_equal ((*c->dst.try_get())[0], "ostreamable: 1, 2");
  assert_string_equal ((*c->dst.try_get())[1], "ostreamable: 1, 2");
  assert_string_equal ((*c->dst.try_get())[2], "ostreamable: 1, 2");

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void std_string_cp (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  std::string str ("paco");
  err = log_warning ("{}", malcpp::strcp (str));
  assert_int_equal (err.own, bl_ok);

  err = log_warning ("{}", malcpp::strcp (std::move (str)));
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);
  assert_int_equal (c->dst.try_get()->size(), 2);
  assert_string_equal ((*c->dst.try_get())[0], "paco");
  assert_string_equal ((*c->dst.try_get())[1], "paco");

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void filtered_out_no_side_effects (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  int side_effect = 0;
  err = log_warning_if (false, "{}", ++side_effect);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (0, side_effect);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void volatile_variable_logging (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  volatile int vol = 0;
  err = log_warning ("{}", vol);
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void timestamp_enabled_test (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = false;
  cfg.producer.timestamp = true;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  err = log_error(
    "{} {} {}", (bl_i64) -1ll, (bl_u16) 20000, malcpp::ostr (ostreamable_type())
    );
  assert_int_equal (err.own, bl_ok);

  err = c->log.run_consume_task (10000);
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (c->dst.try_get()->size(), 1);
  assert_string_equal ((*c->dst.try_get())[0], "-1 20000 ostreamable: 1, 2");

  termination_check (c);
}
/*----------------------------------------------------------------------------*/
static void flush_test (void **state)
{
  context* c = (context*) *state;
  malcpp::cfg cfg;
  bl_err err = c->log.get_cfg (cfg);
  assert_int_equal (err.own, bl_ok);

  cfg.consumer.start_own_thread = true;
  cfg.producer.timestamp = true;

  err = c->log.init (cfg);
  assert_int_equal (err.own, bl_ok);

  err = log_error ("msg");
  assert_int_equal (err.own, bl_ok);

  err = c->log.flush();
  assert_int_equal (err.own, bl_ok);

  assert_int_equal (c->dst.try_get()->size(), 1);
  assert_string_equal ((*c->dst.try_get())[0], "msg");
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
  cmocka_unit_test_setup_teardown (string_shared_ptr, setup, teardown),
  cmocka_unit_test_setup_teardown (vector_shared_ptr, setup, teardown),
  cmocka_unit_test_setup_teardown(
    vector_shared_ptr_truncation, setup, teardown
    ),
  cmocka_unit_test_setup_teardown (string_weak_ptr, setup, teardown),
  cmocka_unit_test_setup_teardown (vector_weak_ptr, setup, teardown),
  cmocka_unit_test_setup_teardown (string_unique_ptr, setup, teardown),
  cmocka_unit_test_setup_teardown (vector_unique_ptr, setup, teardown),
  cmocka_unit_test_setup_teardown (ostreamable_type_by_value, setup, teardown),
  cmocka_unit_test_setup_teardown(
    ostreamable_type_by_unique_ptr, setup, teardown
    ),
  cmocka_unit_test_setup_teardown(
    ostreamable_type_by_shared_ptr, setup, teardown
    ),
  cmocka_unit_test_setup_teardown(
    ostreamable_type_by_weak_ptr, setup, teardown
    ),
  cmocka_unit_test_setup_teardown (std_string_cp, setup, teardown),
  cmocka_unit_test_setup_teardown(
    filtered_out_no_side_effects, setup, teardown
    ),
  cmocka_unit_test_setup_teardown (volatile_variable_logging, setup, teardown),
  cmocka_unit_test_setup_teardown (timestamp_enabled_test, setup, teardown),
  cmocka_unit_test_setup_teardown (flush_test, setup, teardown),
};
/*----------------------------------------------------------------------------*/
int main (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*----------------------------------------------------------------------------*/
