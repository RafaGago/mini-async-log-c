#include <stdio.h>
#include <string.h>

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

/* A program to test malc stability, basically the init/deinit sequence and the
producer side. Thought out to be running for hours, so no file writing. */

malc* ilog = nullptr;

/*----------------------------------------------------------------------------*/
/* Stress destination: a wrapper around "array_dst". Its only purpose is to
  expose externally the received message count at the consumer side.

  It's actually a decent example about how to define/work with own destinations
  too */
/*----------------------------------------------------------------------------*/
typedef struct stress_dst {
  malc_array_dst*  arr;
  uword*           msgs;
  alloc_tbl const* alloc;
  char             lines[32][80];
}
stress_dst;
/*----------------------------------------------------------------------------*/
static bl_err stress_dst_init (void* dst, alloc_tbl const* alloc)
{
  stress_dst* d = (stress_dst*) dst;
  d->msgs  = nullptr;
  d->alloc = alloc;
  d->arr   = (malc_array_dst*) bl_alloc (alloc, malc_array_dst_tbl.size_of);
  if (!d->arr) {
    return bl_alloc;
  }
  bl_err err = bl_ok;
  if (malc_array_dst_tbl.init) {
    err = malc_array_dst_tbl.init (d->arr, alloc);
    if (err) {
      bl_dealloc (alloc, d->arr);
      d->arr = nullptr;
      return err;
    }
  }
  malc_array_dst_set_array(
    d->arr, (char*) d->lines, arr_elems (d->lines), arr_elems (d->lines[0])
    );
  return bl_ok;
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
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
static bl_err stress_dst_idle_task (void* dst)
{
  stress_dst* d = (stress_dst*) dst;
  if (malc_array_dst_tbl.idle_task) {
    return malc_array_dst_tbl.idle_task (d->arr);
  }
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
static bl_err stress_dst_write(
  void* dst, tstamp now, uword sev_val, malc_log_strings const* strs
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
static void stress_dst_set_msg_count_ptr (stress_dst* d, uword* ptr)
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
  atomic_uword ready;
  uword        tls_bytes;
  uword        msgs;
  uword        faults;
  bl_err       err;
  declare_cache_pad_member;
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
    if (c->err) {
      return 1;
    }
  }
  /* Don't include the initialization (TLS creation) time on the data rate
  measurements*/
  atomic_uword_store_rlx (&c->ready, 1);
  while (atomic_uword_load_rlx (&c->ready) == 1) {
    processor_pause();
  }
  for (uword i = 0; i < c->msgs; ++i) {
    log_error (err, "Hello malc, testing {}, {}, {.1}", i, 2, 3.f);
    c->faults += err != bl_ok;
  }
  atomic_uword_store (&c->ready, 3, mo_release);
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
  uword iterations;
  uword msgs;
  uword alloc_mode;
}
pargs;
/*----------------------------------------------------------------------------*/
static void print_usage()
{
  puts("Usage: malc-stress-test <[tls|heap|queue|queue-cpu]> <msgs> <iterations>");
}
/*----------------------------------------------------------------------------*/
static int parse_args(pargs* args, int argc, char const* argv[])
{
  if (argc < 4) {
    fprintf (stderr, "invalid argument count\n");
    print_usage();
    return bl_invalid;
  }
  if (lit_strcmp (argv[1], "tls") == 0) {
    args->alloc_mode = cfg_tls;
  }
  else if (lit_strcmp (argv[1], "heap") == 0) {
    args->alloc_mode = cfg_heap;
  }
  else if (lit_strcmp (argv[1], "queue") == 0) {
    args->alloc_mode = cfg_queue;
  }
  else if (lit_strcmp (argv[1], "queue-cpu") == 0) {
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
      fprintf (stderr, "non numeric message count: %s\n", argv[1]);
      return 1;
  }
  args->iterations = strtol (argv[3], &end, 10);
  if (argv[1] == end) {
      fprintf (stderr, "non numeric iteration count: %s\n", argv[1]);
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
  alloc_tbl           alloc = get_default_alloc(); /* Using malloc and free */
  pargs               args;
  thr_context         tcontext[MAX_THREADS];
  bl_thread           thrs[MAX_THREADS];

  if ((err = parse_args (&args, argc, argv))) {
    return err;
  }

  for (uword i = 0; i < args.iterations; ++i) {
    for (uword thread_idx = 0; thread_idx < arr_elems(threads); ++thread_idx) {
      /* logger allocation/initialization */
      uword thread_count = threads[thread_idx];
      uword faults = 0;
      uword msgs = 0;
      double producer_sec, consumer_sec;
      tstamp start, stop, end;
      stress_dst* sdst = nullptr;
      uword expected_msgs = round_to_next_multiple (args.msgs, thread_count);

      ilog = (malc*) bl_alloc (&alloc,  malc_get_size());
      if (!ilog) {
        fprintf (stderr, "Unable to allocate memory for the malc instance\n");
        return bl_alloc;
      }
      err = malc_create (ilog, &alloc);
      if (err) {
        fprintf (stderr, "Error creating the malc instance\n");
        goto dealloc;
      }
      /* destination register */
      u32 dst_id;
      err = malc_add_destination (ilog, &dst_id, &stress_dst_tbl);
      if (err) {
        fprintf (stderr, "Error creating the stress destination\n");
        goto destroy;
      }
      err = malc_get_destination_instance (ilog, (void**) &sdst, dst_id);
      if (err) {
        fprintf (stderr, "Error getting the destination instance\n");
        return err;
      }
      stress_dst_set_msg_count_ptr (sdst, &msgs);
      /* logger startup */
      malc_cfg cfg;
      err = malc_get_cfg (ilog, &cfg);
      if (err) {
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
        uword div = args.alloc_mode == cfg_queue ? 1 : bl_get_cpu_count();
        cfg.alloc.fixed_allocator_bytes     = QSIZE / div;
        cfg.alloc.fixed_allocator_max_slots = 2;
        cfg.alloc.fixed_allocator_per_cpu = (args.alloc_mode == cfg_queue_cpu);
      }
      err = malc_init (ilog, &cfg);
      if (err) {
        fprintf (stderr, "unable to start logger\n");
        goto destroy;
      }
      /*Thread start*/
      memset(tcontext, 0, sizeof tcontext);
      for (uword th = 0; th < thread_count; ++th) {
        tcontext[th].msgs = expected_msgs / thread_count;
        if (args.alloc_mode == cfg_tls) {
          tcontext[th].tls_bytes = QSIZE / thread_count;
        }
        err = bl_thread_init (&thrs[th], througput_thread, &tcontext[th]);
        if (err) {
          fprintf (stderr, "unable to start a log thread\n");
          /* too lazy now write proper deinitialization for this _test_ program
          under such weird conditions now */
          (void) malc_destroy (ilog);
          bl_dealloc (&alloc, ilog);
          return 1;
        }
      }
      /*Wait for all threads to be ready*/
      for (uword th = 0; th < thread_count; ++th) {
        while (atomic_uword_load_rlx (&tcontext[th].ready) == 0) {
          processor_pause();
        }
      }
      start = bl_get_tstamp();
      /*Signal threads to start*/
      for (uword th = 0; th < thread_count; ++th) {
        atomic_uword_store_rlx (&tcontext[th].ready, 2);
      }
      /*Join (will add jitter)*/
      for (uword th = 0; th < thread_count; ++th) {
        bl_thread_join (&thrs[th]);
      }
      stop = bl_get_tstamp();
      /*Results*/
      for (uword th = 0; th < thread_count; ++th) {
        faults += tcontext[th].faults;
      }
      (void) malc_terminate (ilog, false);
      end = bl_get_tstamp();

      producer_sec  = (double) bl_tstamp_to_nsec (stop - start);
      consumer_sec  = (double) bl_tstamp_to_nsec (end - start);
      producer_sec /= (double) nsec_in_sec;
      consumer_sec /= (double) nsec_in_sec;
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
      if (!err && msgs != (expected_msgs - faults)) {
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
  return err;
}
/*----------------------------------------------------------------------------*/
