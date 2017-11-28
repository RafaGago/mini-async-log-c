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

#include <malc/malc.h>
#include <malc/destinations/file.h>

malc* ilog = nullptr;
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
  /* Don't include the initialization (TLS creation) time on the benchmark */
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
typedef struct pargs {
  uword iterations;
  uword msgs;
}
pargs;
/*----------------------------------------------------------------------------*/
static void print_usage()
{
  puts("Usage: malc-example-benchmark <msgs> <iterations>");
}
/*----------------------------------------------------------------------------*/
static int parse_args(pargs* args, int argc, char const* argv[])
{
  if (argc < 3) {
    print_usage();
    return bl_invalid;
  }
  char* end;
  args->msgs = strtol (argv[1], &end, 10);
  if (argv[1] == end) {
      fprintf (stderr, "non numeric message count: %s\n", argv[1]);
      return 1;
  }
  args->iterations = strtol (argv[2], &end, 10);
  if (argv[1] == end) {
      fprintf (stderr, "non numeric iteration count: %s\n", argv[1]);
      return 1;
  }
  return 0;
}
/*----------------------------------------------------------------------------*/
#define MAX_THREADS 16
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
      uword  thread_count = threads[thread_idx];
      uword  faults = 0;
      double producer_sec, consumer_sec;
      tstamp start, stop, end;

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
      u32 file_id;
      err = malc_add_destination (ilog, &file_id, &malc_file_dst_tbl);
      if (err) {
        fprintf (stderr, "Error creating the file destination\n");
        goto destroy;
      }
      /* logger startup */
      malc_cfg cfg;
      err = malc_get_cfg (ilog, &cfg);
      if (err) {
        fprintf (stderr, "bug when retrieving the logger configuration\n");
        goto destroy;
      }
      cfg.consumer.start_own_thread       = true;
      cfg.alloc.msg_allocator             = nullptr;
      cfg.alloc.slot_size                 = 64;
      cfg.alloc.fixed_allocator_bytes     = 0;
      cfg.alloc.fixed_allocator_max_slots = 0;
      cfg.alloc.fixed_allocator_per_cpu   = 0;

      err = malc_init (ilog, &cfg);
      if (err) {
        fprintf (stderr, "unable to start logger\n");
        goto destroy;
      }
      /*Thread start*/
      memset(tcontext, 0, sizeof tcontext);
      for (uword th = 0; th < thread_count; ++th) {
        tcontext[th].msgs      = args.msgs / thread_count;
        tcontext[th].tls_bytes = (32 * 1024 * 1024) / thread_count;
        err = bl_thread_init (&thrs[th], througput_thread, &tcontext[th]);
        if (err) {
          fprintf (stderr, "unable to start a log thread\n");
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
        ((double) (args.msgs - faults) / producer_sec) / 1000.,
        ((double) (args.msgs - faults) / consumer_sec) / 1000.,
        faults
        );
    destroy:
      (void) malc_destroy (ilog);
    dealloc:
      bl_dealloc (&alloc, ilog);
    }
  }
  return err;
}
/*----------------------------------------------------------------------------*/
