#include <sstream>

#include <thread>
#include <malc/malc_lean.hpp>

malcpp::malcpp<> log;
/*----------------------------------------------------------------------------*/
int main (int argc, char const* argv[])
{
  /* destination register: adding logging to stdout/stderr */
  log.add_destination<malcpp::stdouterr_dst>();
  log.add_destination<malcpp::file_dst>();
  log.init();

  std::thread thr;
  thr = std::thread ([](){
    log_error_i (log, "Hello malc, testing {}, {}, {.1}", 1, 2, 3.f);
  });
  thr.join();
  return 0;
}
/*----------------------------------------------------------------------------*/
