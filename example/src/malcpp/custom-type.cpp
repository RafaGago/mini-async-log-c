//------------------------------------------------------------------------------
// Shows how to add a datatype to log
//------------------------------------------------------------------------------
#include <thread>
#include <list>
#include <malcpp/malcpp.hpp>
#include <malcpp/custom_type.hpp>
//------------------------------------------------------------------------------
class mylogged_type :
  public malcpp::custom_log_type<mylogged_type> { // CRTP static inheritance.
public:
  //----------------------------------------------------------------------------
  mylogged_type()
  {
    ptr = std::make_shared<std::list<int> >(
      std::initializer_list<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }
      );
  };
  //----------------------------------------------------------------------------
  // called via CRTP.
  //
  // la:      an object to push data to the log entry.
  // context: if you called "malcpp::object" providing a void* you get it here.
  // flag:    if you called "malcpp::object" providing a flag you get it here.
  // alloc:   allocator passed to malc on initialization.
  //
  // this function has to return 0 if no error, non-zero otherwise.
  //
  //----------------------------------------------------------------------------
  int dump_value(
    malcpp::log_entry_adder const& la,
    void*                          context,
    bl_u8                          flag,
    bl_alloc_tbl const&            alloc
    )
  {
    if (ptr.get() == nullptr) {
      return la.push_null_smartptr();
    }
    auto& l = *ptr;
    int err = la.push ("a lot of data: ");
    if (err) {
      return err;
    }
    return la.push (std::vector<int> (l.begin(), l.end()));
  }
  //----------------------------------------------------------------------------
private:
  std::shared_ptr<std::list<int> > ptr;
};
//------------------------------------------------------------------------------
malcpp::malcpp<> log;
//------------------------------------------------------------------------------
int main (int argc, char const* argv[])
{
  /* destination register: adding logging to stdout/stderr */
  log.add_destination<malcpp::stdouterr_dst>();
  log.add_destination<malcpp::file_dst>();
  log.init();

  std::thread thr;
  thr = std::thread ([](){
    log_error_i (
      log,
      "logging a custom type: {}",
      malcpp::object (mylogged_type(), mylogged_type::get_table())
      );
  });
  thr.join();
  return 0;
}
//------------------------------------------------------------------------------
