#include <thread>
#include <malc/malc.hpp>

malcpp::malcpp<> log;
/*----------------------------------------------------------------------------*/
static inline decltype(log)& get_malc_logger_instance()
{
  return log;
}
/*----------------------------------------------------------------------------*/
void log_thread()
{
  bl_err err;
  log.producer_thread_local_init (128 * 1024);
  err = log_error ("Hello malc");
  err = log_warning ("testing {}, {}, {.1}", 1, 2, 3.f);
}
/*----------------------------------------------------------------------------*/
int main (int argc, char const* argv[])
{
  /* destination register */
  auto stdouterr = log.add_destination<malcpp::stdouterr_dst>();

  /* generic malc instance destination configuration */
  malcpp::dst_cfg dcfg = stdouterr.get_cfg();
  dcfg.show_timestamp  = false;
  dcfg.show_severity   = false;
  dcfg.severity        = malcpp::sev_warning;
  stdouterr.set_cfg (dcfg);

  /* destination specific configuration*/
  (void) stdouterr.get().set_stderr_severity (malcpp::sev_debug);

  /* destination register */
  auto file = log.add_destination<malcpp::file_dst>();

  /* generic malc instance destination configuration */
  dcfg = file.get_cfg();
  dcfg.show_timestamp = true;
  dcfg.show_severity  = true;
  dcfg.severity       = malcpp::sev_debug;
  file.set_cfg (dcfg);

  /* destination specific configuration*/
  malcpp::file_dst_cfg fcfg;
  (void) file.get().get_cfg (fcfg);
  fcfg.prefix = "malc-cpp-wrapper-example";
  (void) file.get().set_cfg (fcfg);

  /* logger startup */
  malcpp::cfg cfg = log.get_cfg();
  cfg.consumer.start_own_thread = true; /* this thread runs the event-loop */
  log.init (cfg);

  /* threads can start logging */
  int err = 0;
  std::thread thr;
  thr = std::thread (&log_thread);
  thr.join();
  return err;
}
/*----------------------------------------------------------------------------*/
