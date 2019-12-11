/* A program to test malc stability, basically the init/deinit sequence and the
producer side. Thought out to be running for hours. It adds a destination on RAM
with extra instrumentation. */

#include <stdio.h>
#include <string.h>

#define BL_UNPREFIXED_PRINTF_FORMATS
#include <bl/base/default_allocator.h>
#include <bl/base/thread.h>
#include <bl/base/cache.h>
#include <bl/base/atomic.h>
#include <bl/base/utility.h>
#include <bl/base/processor_pause.h>
#include <bl/base/time.h>
#include <bl/base/integer_printf_format.h>
#include <bl/base/utility.h>
#include <bl/base/static_integer_math.h>

#include <malc/malc.h>
#include <malc/destinations/array.h>

malc* ilog = nullptr;
/*----------------------------------------------------------------------------*/
/* Stress destination: a wrapper around "array_dst". Its only purpose is to
  expose externally the received message count at the consumer side at the end.

  "stree_dst" is actually a decent example about how to define/work with own
  destinations*/
/*----------------------------------------------------------------------------*/
typedef struct stress_dst {
  malc_array_dst*     arr;
  bl_uword*           msgs;
  bl_alloc_tbl const* alloc;
  char                lines[32][80];
}
stress_dst;
/*----------------------------------------------------------------------------*/
static bl_err stress_dst_init (void* dst, bl_alloc_tbl const* alloc)
{
  stress_dst* d = (stress_dst*) dst;
  d->msgs  = nullptr;
  d->alloc = alloc;
  d->arr   = (malc_array_dst*) bl_alloc (alloc, malc_array_dst_tbl.size_of);
  if (!d->arr) {
    return bl_mkerr (bl_alloc);
  }
  bl_err err = bl_mkok();
  if (malc_array_dst_tbl.init) {
    err = malc_array_dst_tbl.init (d->arr, alloc);
    if (err.own) {
      bl_dealloc (alloc, d->arr);
      d->arr = nullptr;
      return err;
    }
  }
  malc_array_dst_set_array(
    d->arr, (char*) d->lines,bl_arr_elems (d->lines),bl_arr_elems (d->lines[0])
    );
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static void stress_dst_terminate (void* dst)
{
  stress_dst* d = (stress_dst*) dst;
  if (malc_array_dst_tbl.terminate) {
    malc_array_dst_tbl.terminate (d->arr);
  }
  bl_dealloc (d->alloc, d->arr);
}
/*----------------------------------------------------------------------------*/
static bl_err stress_dst_flush (void* dst)
{
  stress_dst* d = (stress_dst*) dst;
  if (malc_array_dst_tbl.flush) {
    return malc_array_dst_tbl.flush (d->arr);
  }
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static bl_err stress_dst_idle_task (void* dst)
{
  stress_dst* d = (stress_dst*) dst;
  if (malc_array_dst_tbl.idle_task) {
    return malc_array_dst_tbl.idle_task (d->arr);
  }
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static bl_err stress_dst_write(
  void* dst, bl_timept64 now, unsigned sev_val, malc_log_strings const* strs
  )
{
  stress_dst* d = (stress_dst*) dst;
  ++(*d->msgs);
  return malc_array_dst_tbl.write (d->arr, now, sev_val, strs);
}
/*----------------------------------------------------------------------------*/
static const malc_dst stress_dst_tbl = {
  sizeof (stress_dst),
  stress_dst_init,
  stress_dst_terminate,
  stress_dst_flush,
  stress_dst_idle_task,
  stress_dst_write
};
/*----------------------------------------------------------------------------*/
static void stress_dst_set_msg_count_ptr (stress_dst* d, bl_uword* ptr)
{
  d->msgs = ptr;
}
/*----------------------------------------------------------------------------*/
static inline malc* get_malc_logger_instance()
{
  return ilog;
}
/*----------------------------------------------------------------------------*/
typedef struct thr_context {
  bl_atomic_uword ready;
  bl_uword        tls_bytes;
  bl_uword        msgs;
  bl_uword        faults;
  bl_err          err;
  bl_declare_cache_pad_member;
}
thr_context;
/*----------------------------------------------------------------------------*/
static int througput_thread (void* ctx)
{
  bl_err err;
  thr_context* c = (thr_context*) ctx;

  if (c->tls_bytes) {
    c->err = malc_producer_thread_local_init(
      get_malc_logger_instance(), c->tls_bytes
      );
    if (c->err.own) {
      return 1;
    }
  }
  /* Don't include the initialization (TLS creation) time on the data rate
  measurements*/
  bl_atomic_uword_store_rlx (&c->ready, 1);
  while (bl_atomic_uword_load_rlx (&c->ready) == 1) {
    bl_processor_pause();
  }
  for (bl_uword i = 0; i < c->msgs; ++i) {
    err = log_error ("Hello malc, testing {}, {}, {.1}", i, 2, 3.f);
    c->faults += err.own != bl_ok;
  }
  bl_atomic_uword_store (&c->ready, 3, bl_mo_release);
  return 0;
}
/*----------------------------------------------------------------------------*/
enum cfg_mode {
  cfg_tls,
  cfg_heap,
  cfg_queue,
  cfg_queue_cpu,
};
/*----------------------------------------------------------------------------*/
typedef struct pargs {
  bl_uword iterations;
  bl_uword msgs;
  bl_uword alloc_mode;
}
pargs;
/*----------------------------------------------------------------------------*/
static void print_usage()
{
  puts(
    "Usage: malc-stress-test <[tls|heap|queue|queue-cpu]> <msgs> <iterations>"
    );
}
/*----------------------------------------------------------------------------*/
static int parse_args(pargs* args, int argc, char const* argv[])
{
  if (argc < 4) {
    fprintf (stderr, "invalid argument count\n");
    print_usage();
    return bl_invalid;
  }
  if (bl_lit_strcmp (argv[1], "tls") == 0) {
    args->alloc_mode = cfg_tls;
  }
  else if (bl_lit_strcmp (argv[1], "heap") == 0) {
    args->alloc_mode = cfg_heap;
  }
  else if (bl_lit_strcmp (argv[1], "queue") == 0) {
    args->alloc_mode = cfg_queue;
  }
  else if (bl_lit_strcmp (argv[1], "queue-cpu") == 0) {
    args->alloc_mode = cfg_queue_cpu;
  }
  else {
    fprintf (stderr, "unknown mode: %s\n", argv[1]);
    print_usage();
    return bl_invalid;
  }
  char* end;
  args->msgs = strtol (argv[2], &end, 10);
  if (argv[1] == end) {
      fprintf (stderr, "non numeric message count: %s\n", argv[2]);
      return 1;
  }
  args->iterations = strtol (argv[3], &end, 10);
  if (argv[1] == end) {
      fprintf (stderr, "non numeric iteration count: %s\n", argv[3]);
      return 1;
  }
  return 0;
}
/*----------------------------------------------------------------------------*/
#define MAX_THREADS 16
#define QSIZE (32 * 1024 * 1024)
static const int threads[] = { 1, 2, 4, 8, MAX_THREADS };
/*----------------------------------------------------------------------------*/
int main (int argc, char const* argv[])
{
  bl_err              err;
  bl_alloc_tbl        alloc = bl_get_default_alloc(); /* Using malloc and free */
  pargs               args;
  thr_context         tcontext[MAX_THREADS];
  bl_thread           thrs[MAX_THREADS];

  err.own = parse_args (&args, argc, argv);
  if (err.own) {
    return err.own;
  }
  for (bl_uword i = 0; i < args.iterations; ++i) {
    for(
      bl_uword thread_idx = 0; thread_idx < bl_arr_elems(threads); ++thread_idx
      ) {
      /* logger allocation/initialization */
      bl_uword thread_count = threads[thread_idx];
      bl_uword faults = 0;
      bl_uword msgs = 0;
      double producer_sec, consumer_sec;
      bl_timept64 start, stop, end;
      stress_dst* sdst = nullptr;
      bl_uword expected_msgs =
        bl_round_to_next_multiple (args.msgs, thread_count);

      ilog = (malc*) bl_alloc (&alloc,  malc_get_size());
      if (!ilog) {
        fprintf (stderr, "Unable to allocate memory for the malc instance\n");
        return bl_alloc;
      }
      err = malc_create (ilog, &alloc);
      if (err.own) {
        fprintf (stderr, "Error creating the malc instance\n");
        goto dealloc;
      }
      /* destination register */
      size_t dst_id;
      err = malc_add_destination (ilog, &dst_id, &stress_dst_tbl);
      if (err.own) {
        fprintf (stderr, "Error creating the stress destination\n");
        goto destroy;
      }
      err = malc_get_destination_instance (ilog, (void**) &sdst, dst_id);
      if (err.own) {
        fprintf (stderr, "Error getting the destination instance\n");
        return err.own;
      }
      stress_dst_set_msg_count_ptr (sdst, &msgs);
      /* logger startup */
      malc_cfg cfg;
      err = malc_get_cfg (ilog, &cfg);
      if (err.own) {
        fprintf (stderr, "bug when retrieving the logger configuration\n");
        goto destroy;
      }
      cfg.consumer.start_own_thread = true;

      if (args.alloc_mode != cfg_heap) {
        cfg.alloc.msg_allocator = nullptr;
      }
      cfg.alloc.slot_size                 = 64;
      cfg.alloc.fixed_allocator_bytes     = 0;
      cfg.alloc.fixed_allocator_max_slots = 0;
      cfg.alloc.fixed_allocator_per_cpu   = 0;

      if (args.alloc_mode == cfg_queue || args.alloc_mode == cfg_queue_cpu) {
        bl_uword div = args.alloc_mode == cfg_queue ? 1 : bl_get_cpu_count();
        cfg.alloc.fixed_allocator_bytes     = QSIZE / div;
        cfg.alloc.fixed_allocator_max_slots = 2;
        cfg.alloc.fixed_allocator_per_cpu = (args.alloc_mode == cfg_queue_cpu);
      }
      err = malc_init (ilog, &cfg);
      if (err.own) {
        fprintf (stderr, "unable to start logger\n");
        goto destroy;
      }
      /*Thread start*/
      memset(tcontext, 0, sizeof tcontext);
      for (bl_uword th = 0; th < thread_count; ++th) {
        tcontext[th].msgs = expected_msgs / thread_count;
        if (args.alloc_mode == cfg_tls) {
          tcontext[th].tls_bytes = QSIZE / thread_count;
        }
        err = bl_thread_init (&thrs[th], througput_thread, &tcontext[th]);
        if (err.own) {
          fprintf (stderr, "unable to start a log thread\n");
          /* too lazy now to write proper deinitialization for this _test_
          program under such unlikely to happen conditions. Every thread should
          be cleanly killed */
          (void) malc_destroy (ilog);
          bl_dealloc (&alloc, ilog);
          return 1;
        }
      }
      /*Wait for all threads to be ready*/
      for (bl_uword th = 0; th < thread_count; ++th) {
        while (bl_atomic_uword_load_rlx (&tcontext[th].ready) == 0) {
          bl_processor_pause();
        }
      }
      start = bl_timept64_get();
      /*Signal threads to start*/
      for (bl_uword th = 0; th < thread_count; ++th) {
        bl_atomic_uword_store_rlx (&tcontext[th].ready, 2);
      }
      /*Join (will add jitter)*/
      for (bl_uword th = 0; th < thread_count; ++th) {
        bl_thread_join (&thrs[th]);
      }
      stop = bl_timept64_get();
      (void) malc_terminate (ilog, false);
      end = bl_timept64_get();

      /*Results*/
      for (bl_uword th = 0; th < thread_count; ++th) {
        faults += tcontext[th].faults;
      }
      producer_sec  = (double) bl_timept64_to_nsec (stop - start);
      consumer_sec  = (double) bl_timept64_to_nsec (end - start);
      producer_sec /= (double) bl_nsec_in_sec;
      consumer_sec /= (double) bl_nsec_in_sec;
      printf(
        "threads: %2" FMT_W ", Producer Kmsgs/sec: %.2f, Consumer Kmsgs/sec: %.2f, faults: %" FMT_UW "\n",
        thread_count,
        ((double) (expected_msgs - faults) / producer_sec) / 1000.,
        ((double) (expected_msgs - faults) / consumer_sec) / 1000.,
        faults
        );
    destroy:
      (void) malc_destroy (ilog);
    dealloc:
      bl_dealloc (&alloc, ilog);
      if (!err.own && msgs != (expected_msgs - faults)) {
        fprintf(
          stderr,
          "BUG! %" FMT_UW " messages were lost\n",
          args.msgs - faults - msgs
          );
        bl_assert (false);
        return -1;
      }
    }
  }
  return err.own;
}
/*----------------------------------------------------------------------------*/
