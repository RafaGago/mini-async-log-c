#include <stdio.h>

#include <bl/base/default_allocator.h>
#include <bl/base/thread.h>
#include <bl/base/atomic.h>

#include <malc/malc.h>
#include <malc/destinations/file.h>
#include <malc/destinations/stdouterr.h>

malc* ilog = nullptr;
/*----------------------------------------------------------------------------*/
static inline malc* get_malc_logger_instance()
{
  return ilog;
}
/*----------------------------------------------------------------------------*/
int log_thread (void* ctx)
{
  bl_atomic_uword* alive = (bl_atomic_uword*) ctx;
  bl_err err = malc_producer_thread_local_init (ilog, 128 * 1024);
  if (err.bl) {
    err = log_error ("Error when starting thread local storage: {}", err.bl);
  }
  err = log_error ("Hello from log thread");
  bl_uword alive_prev = bl_atomic_uword_fetch_sub_rlx (alive, 1);
  if (alive_prev == 1) {
    /* the last thread alive can invoke malc-terminate, malc_terminate could
     be invoked by any other thread (e.g. the main one). */
    (void) malc_terminate (ilog, false);
  }
  return 0;
}
/*----------------------------------------------------------------------------*/
int main (int argc, char const* argv[])
{
  bl_err                err;
  bl_alloc_tbl          alloc = bl_get_default_alloc(); /* Using malloc and free */
  static const bl_uword thread_count = 4;
  bl_atomic_uword       alive_threads = thread_count;

  /* logger allocation/initialization */
  ilog = bl_alloc (&alloc,  malc_get_size());
  if (!ilog) {
    fprintf (stderr, "Unable to allocate memory for the malc instance\n");
    return bl_alloc;
  }
  err = malc_create (ilog, &alloc);
  if (err.bl) {
    fprintf (stderr, "Error creating the malc instance\n");
    goto dealloc;
  }

  /* destination register */
  bl_u32 stdouterr_id;
  bl_u32 file_id;
  err = malc_add_destination (ilog, &stdouterr_id, &malc_stdouterr_dst_tbl);
  if (err.bl) {
    fprintf (stderr, "Error creating the stdout/stderr destination\n");
    goto destroy;
  }
  err = malc_add_destination (ilog, &file_id, &malc_file_dst_tbl);
  if (err.bl) {
    fprintf (stderr, "Error creating the file destination\n");
    goto destroy;
  }

  /* logger startup */
  malc_cfg cfg;
  err = malc_get_cfg (ilog, &cfg);
  if (err.bl) {
    fprintf (stderr, "bug when retrieving the logger configuration\n");
    goto destroy;
  }
  cfg.consumer.start_own_thread = false; /* main tread runs the event-loop*/
  err = malc_init (ilog, &cfg);
  if (err.bl) {
    fprintf (stderr, "unable to start logger\n");
    goto destroy;
  }

  for (bl_uword i = 0; i < thread_count; ++i) {
    bl_thread t;
    err = bl_thread_init (&t, log_thread, (void*) &alive_threads);
    if (err.bl) {
      fprintf (stderr, "unable to start log thread 1\n");
      goto destroy;
    }
  }

  do {
    /* run event-loop to consume malc messages */
    err = malc_run_consume_task (ilog, 10000);
  }
  while (!err.bl || err.bl == bl_nothing_to_do);
  err = bl_mkok();

destroy:
  (void) malc_destroy (ilog);
dealloc:
  bl_dealloc (&alloc, ilog);
  return err.bl;
}
/*----------------------------------------------------------------------------*/
