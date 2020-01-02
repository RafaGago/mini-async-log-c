//------------------------------------------------------------------------------
// A program to test malc stability, basically the init/deinit sequence and the
// producer side. Thought out to be running for hours. It adds a destination on RAM
// with extra instrumentation.
//------------------------------------------------------------------------------
#if defined (_MSC_VER)
  #define _CRTDBG_MAP_ALLOC
  #include <stdlib.h>
  #include <crtdbg.h>
#endif

#include <atomic>
#include <array>
#include <chrono>
#include <map>
#include <thread>

#include <stdio.h>
#include <string.h>

#include <bl/base/processor_pause.h>
#include <malcpp/malcpp_lean.hpp>
//------------------------------------------------------------------------------
malcpp::malcpp<true, false, false> ilog;
//------------------------------------------------------------------------------
static inline decltype (ilog)& get_malc_instance()
{
  return ilog;
}
//------------------------------------------------------------------------------
// Stress destination to count messages
//------------------------------------------------------------------------------
class message_count_destination {
public:
  /*--------------------------------------------------------------------------*/
  message_count_destination (const bl_alloc_tbl& alloc) {};
  /*--------------------------------------------------------------------------*/
  bool flush()     { return true; }
  bool idle_task() { return true; }
  /*--------------------------------------------------------------------------*/
  bool write (bl_u64 nsec, bl_uword severity, malcpp::log_strings const& strs)
  {
    ++(*m_msgs);
    return true;
  }
  /*--------------------------------------------------------------------------*/
  void set_message_counter (std::size_t& var)
  {
    m_msgs = &var;
  }
  /*--------------------------------------------------------------------------*/
private:
  std::size_t* m_msgs;
};
//------------------------------------------------------------------------------
struct unpadded_thr_context {
  std::atomic<std::size_t> init{0};
  std::size_t              tls_bytes;
  std::size_t              msgs;
  std::size_t              faults;
  bl_err                   err;
};
struct thr_context : public unpadded_thr_context {
  char padding [64 - sizeof (unpadded_thr_context) ];
};
//------------------------------------------------------------------------------
enum thread_launch_state {
  tl_launched,
  tl_ready,
  tl_clear_to_log,
  tl_done,
};
//------------------------------------------------------------------------------
static void througput_thread (thr_context& c)
{
  bl_err err;
  if (c.tls_bytes > 0) {
    try {
      ilog.producer_thread_local_init (c.tls_bytes);
    }
    catch(...) {
      c.err = bl_mkerr (bl_error);
      return;
    }
  }
  /* Don't include the initialization (TLS creation) time on the data rate
  measurements */
  c.init.store (tl_ready, std::memory_order_relaxed);
  while (c.init.load (std::memory_order_relaxed) != tl_clear_to_log) {
    bl_processor_pause();
  }
  for (std::size_t i = 0; i < c.msgs; ++i) {
    err = log_error ("Hello malc, testing {}, {}, {.1}", i, 2, 3.f);
    c.faults += (err.own != bl_ok);
  }
  c.init.store (tl_done, std::memory_order_release);
}
//------------------------------------------------------------------------------
enum cfg_mode {
  cfg_tls,
  cfg_heap,
  cfg_queue,
  cfg_queue_cpu,
};
//------------------------------------------------------------------------------
typedef struct pargs {
  std::size_t iterations;
  std::size_t msgs;
  std::size_t alloc_mode;
}
pargs;
//------------------------------------------------------------------------------
static void print_usage()
{
  puts(
    "Usage: malc-stress-test <[tls|heap|queue|queue-cpu]> <msgs> <iterations>"
    );
}
//------------------------------------------------------------------------------
static bool parse_args (pargs& args, int argc, char const* argv[])
{
  if (argc < 4) {
    fprintf (stderr, "invalid argument count\n");
    print_usage();
    return false;
  }

  std::map<std::string, std::size_t> mode = {
    { "tls", cfg_tls },
    { "heap", cfg_heap },
    { "queue", cfg_queue },
    { "queue-cpu", cfg_queue_cpu },
  };

  auto m = mode.find (argv[1]);
  if (m == mode.end()) {
    fprintf (stderr, "unknown mode: %s\n", argv[1]);
    print_usage();
    return false;
  }
  args.alloc_mode = m->second;

  auto parse_uint = [](std::size_t& v, char const* str, char const* errstr) {
    char* end;
    auto r = std::strtoll (str, &end, 10);
    if (str == end || r < 0) {
      fprintf (stderr, "%s: %s\n", errstr, str);
      return false;
    }
    v = r;
    return true;
  };
  if (!parse_uint (args.msgs, argv[2], "invalid message count")) {
    return false;
  }
  return parse_uint (args.iterations, argv[3], "invalid iteration count");
}
//------------------------------------------------------------------------------
static constexpr unsigned max_threads = 16;
static constexpr unsigned qsize = 32 * 1024 * 1024;
static constexpr int threads[] = { 1, 2, 4, 8, max_threads };
//------------------------------------------------------------------------------
static void configure_malcpp (malcpp::malc_cfg& cfg, pargs const& args)
{
  cfg.consumer.start_own_thread = true;
  if (args.alloc_mode != cfg_heap) {
    cfg.alloc.msg_allocator = nullptr;
  }
  cfg.alloc.slot_size                 = 64;
  cfg.alloc.fixed_allocator_bytes     = 0;
  cfg.alloc.fixed_allocator_max_slots = 0;
  cfg.alloc.fixed_allocator_per_cpu   = 0;

  if (args.alloc_mode == cfg_queue || args.alloc_mode == cfg_queue_cpu) {
    std::size_t div =
      (args.alloc_mode == cfg_queue) ? 1 : std::thread::hardware_concurrency();
    cfg.alloc.fixed_allocator_bytes     = qsize / div;
    cfg.alloc.fixed_allocator_max_slots = 2;
    cfg.alloc.fixed_allocator_per_cpu = (args.alloc_mode == cfg_queue_cpu);
  }
}
//------------------------------------------------------------------------------
int main (int argc, char const* argv[])
{
  pargs  args;
  std::array<thr_context, max_threads> tcontext {};
  std::array<std::thread, max_threads> thrs {};

  if (!parse_args (args, argc, argv)) {
    return 1;
  }
  for (std::size_t i = 0; i < args.iterations; ++i) {
    for (
      std::size_t thread_idx = 0;
      thread_idx < sizeof threads / sizeof threads[0];
      ++thread_idx
      ) {

      std::size_t thread_count = threads[thread_idx];
      std::size_t msgs = 0;
      std::size_t expected_msgs =
        bl_round_to_next_multiple (args.msgs, thread_count);

      ilog.construct();
      auto destructor = ilog.get_scoped_destructor();
      auto dst = ilog.add_destination<message_count_destination>();
      dst.get().set_message_counter (msgs);

      /* logger startup */
      auto cfg = ilog.get_cfg();
      configure_malcpp (cfg, args);
      ilog.init (cfg);

      /*Thread start*/
      for (std::size_t th = 0; th < thread_count; ++th) {
        tcontext[th].msgs = expected_msgs / thread_count;
        if (args.alloc_mode == cfg_tls) {
          tcontext[th].tls_bytes = qsize / thread_count;
        }
        thrs[th] = std::thread (througput_thread, std::ref (tcontext[th]));
      }
      /*Wait for all threads to be ready*/
      for (std::size_t th = 0; th < thread_count; ++th) {
        while(
          tcontext[th].init.load (std::memory_order_relaxed) != tl_ready
          ) {
          if (tcontext[th].err.own != 0) {
            fprintf(stderr, "error when launching one of the threads\n");
            return 1;
          }
          bl_processor_pause();
        }
      }
      auto start = std::chrono::steady_clock::now();
      /*Signal threads to start*/
      for (std::size_t th = 0; th < thread_count; ++th) {
        tcontext[th].init.store (tl_clear_to_log, std::memory_order_relaxed);
      }
      /*Join (will add jitter)*/
      for (std::size_t th = 0; th < thread_count; ++th) {
        thrs[th].join();
      }
      auto stop = std::chrono::steady_clock::now();
      ilog.terminate (false);
      auto end = std::chrono::steady_clock::now();

      /*Results*/
      std::size_t faults = 0;
      for (std::size_t th = 0; th < thread_count; ++th) {
        faults += tcontext[th].faults;
      }
      auto producer_sec = std::chrono::duration<double> (stop - start).count();
      auto consumer_sec = std::chrono::duration<double> (end - start).count();

      printf(
        "threads: %2d, Producer Kmsgs/sec: %.2f, Consumer Kmsgs/sec: %.2f, faults: %2llu\n",
        (int) thread_count,
        ((double) (expected_msgs - faults) / producer_sec) / 1000.,
        ((double) (expected_msgs - faults) / consumer_sec) / 1000.,
        (unsigned long long) faults
        );
    }
  }
  return 0;
}
//------------------------------------------------------------------------------
