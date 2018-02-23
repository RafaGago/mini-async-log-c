#include <stdio.h>

#include <bl/base/default_allocator.h>
#include <bl/base/thread.h>

#include <malc/destinations/file.h>
#include <malc/destinations/stdouterr.h>
#include <malc/malc.hpp>

malc_owner log;
/*----------------------------------------------------------------------------*/
static inline malc* get_malc_logger_instance()
{
  return log.handle();
}
/*----------------------------------------------------------------------------*/
int log_thread (void* ctx)
{
  bl_err err;
  log_error (err, "Hello malc, testing {}, {}, {.1}", 1, 2, 3.f);
  (void) log.terminate (true); /* terminating the logger. Will force the
                                  event loop on main to exit */
  return 0;
}
/*----------------------------------------------------------------------------*/
int main (int argc, char const* argv[])
{
  /* destination register */
  bl_err err;
  u32 stdouterr_id;
  u32 file_id;
  err = log.add_destination (stdouterr_id, malc_stdouterr_dst_tbl);
  if (err.bl) {
    fprintf (stderr, "Error creating the stdout/stderr destination\n");
    return err.bl;
  }
  err = log.add_destination (file_id, malc_file_dst_tbl);
  if (err.bl) {
    fprintf (stderr, "Error creating the file destination\n");
    return err.bl;
  }
  /* logger startup */
  malc_cfg cfg;
  err = log.get_cfg (cfg);
  if (err.bl) {
    fprintf (stderr, "bug when retrieving the logger configuration\n");
    return err.bl;
  }
  cfg.consumer.start_own_thread = false; /* main tread runs the event-loop*/
  err = log.init (cfg);
  if (err.bl) {
    fprintf (stderr, "unable to start logger\n");
    return err.bl;
  }
  /* threads can start logging */
  bl_thread t;
  err = bl_thread_init (&t, log_thread, nullptr);
  if (err.bl) {
    fprintf (stderr, "unable to start a log thread\n");
    return err.bl;
  }
  do {
    /* run event-loop to consume malc messages */
    err = log.run_consume_task (10000);
  }
  while (!err.bl || err.bl == bl_nothing_to_do);
  return 0;
}
/*----------------------------------------------------------------------------*/
