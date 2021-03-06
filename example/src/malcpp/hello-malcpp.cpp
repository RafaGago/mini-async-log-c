//------------------------------------------------------------------------------
// Minimal hello world.
//------------------------------------------------------------------------------
#include <thread>
#include <malcpp/malcpp_lean.hpp>

malcpp::malcpp<> logger;
//------------------------------------------------------------------------------
int main (int argc, char const* argv[])
{
  /* destination register: adding logging to stdout/stderr */
  logger.add_destination<malcpp::stdouterr_dst>();
  logger.add_destination<malcpp::file_dst>();
  auto cfg = logger.get_cfg();
  cfg.consumer.start_own_thread = true;
  logger.init (cfg);

  std::thread thr;
  thr = std::thread ([](){
    log_error_i (logger, "Hello malc, testing {}, {}, {.1}", 1, 2, 3.f);
  });
  thr.join();
  return 0;
}
//------------------------------------------------------------------------------
