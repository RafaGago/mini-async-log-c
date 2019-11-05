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
  malc_producer_thread_local_init (log.handle(), 128 * 1024);
  log_error (err, "Hello malc");
  log_debug (err, "testing {}, {}, {.1}", 1, 2, 3.f);
  (void) log.terminate (true); /* terminating the logger. Will force the
                                  event loop on main's thread to exit */
  return 0;
}
/*----------------------------------------------------------------------------*/
int main (int argc, char const* argv[])
{
  /* destination register */
  bl_err err;
  auto stdouterr = log.add_destination<malc_stdouterr_dst_adapter>();
  if (!stdouterr.is_valid()) {
    fprintf (stderr, "Error creating the stdout/stderr destination\n");
    return 1;
  }
  auto file = log.add_destination<malc_file_dst_adapter>();
  if (!file.is_valid()) {
    fprintf (stderr, "Error creating the file destination\n");
    return 1;
  }
  /* deliberately ignoring error codes on already created objects, as this
  non allocating calls won't fail in this circumstances */

  /* generic malc instance destination configuration */
  malc_dst_cfg dcfg;

  stdouterr.get_cfg (dcfg);
  dcfg.show_timestamp = false;
  dcfg.show_severity  = false;
  dcfg.severity       = malc_sev_warning;
  stdouterr.set_cfg (dcfg);

  file.get_cfg (dcfg);
  dcfg.show_timestamp = true;
  dcfg.show_severity  = true;
  dcfg.severity       = malc_sev_debug;
  file.set_cfg (dcfg);

  /* instance specific configuration*/
  stdouterr.try_get()->set_stderr_severity (malc_sev_debug);

  malc_file_cfg fcfg;
  file.try_get()->get_cfg (fcfg);
  fcfg.prefix = "malc-cpp-wrapper-example";
  file.try_get()->set_cfg (fcfg);

  /* logger startup */
  malc_cfg cfg;
  err = log.get_cfg (cfg);
  if (err.bl) {
    fprintf (stderr, "bug when retrieving the logger configuration\n");
    return err.bl;
  }
  cfg.consumer.start_own_thread = false; /* this thread runs the event-loop */
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
