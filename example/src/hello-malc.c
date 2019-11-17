#include <stdio.h>

#include <bl/base/default_allocator.h>
#include <bl/base/thread.h>

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
  bl_err err = log_error ("Hello malc, testing {}, {}, {.1}", 1, 2, 3.f);
  (void) malc_terminate (ilog, false); /* terminating the logger. Will force the
                                          event loop on main's thread to exit */
  return 0;
}
/*----------------------------------------------------------------------------*/
int main (int argc, char const* argv[])
{
  bl_err       err;
  bl_alloc_tbl alloc = bl_get_default_alloc(); /* Using malloc and free */

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
  bl_u32    stdouterr_id;
  bl_u32    file_id;
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
  cfg.consumer.start_own_thread = false; /* this thread runs the event-loop*/
  err = malc_init (ilog, &cfg);
  if (err.bl) {
    fprintf (stderr, "unable to start logger\n");
    goto destroy;
  }

  /* threads can start logging */
  bl_thread t;
  err = bl_thread_init (&t, log_thread, nullptr);
  if (err.bl) {
    fprintf (stderr, "unable to start a log thread\n");
    goto destroy;
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
