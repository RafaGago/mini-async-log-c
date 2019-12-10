#include <limits>
#include <malc/custom_type.hpp>

namespace malcpp {

//------------------------------------------------------------------------------
malc_obj_log_data obj_log_data::get_as_str (char const* str, std::size_t len)
{
  malc_obj_log_data ld;
  ld.is_str = 1;
  ld.data.str.ptr = str;
  ld.data.str.len = len;
  if (ld.data.str.len < len) {
    /* overflow: truncating */
    ld.data.str.len = std::numeric_limits<decltype (ld.data.str.len)>::max();
  }
  return ld;
}
//------------------------------------------------------------------------------
malc_obj_log_data obj_log_data::get(
  void const* addr, std::size_t count, bl_u8 builtin_type
  )
{
  malc_obj_log_data ld;
  ld.is_str = 0;
  ld.data.builtin.ptr   = addr;
  ld.data.builtin.count = count;
  ld.data.builtin.type  = builtin_type;
  if (ld.data.builtin.count < count) {
    /* overflow: truncating. (Notice that returning more values is possible
    by calling a loop, just not done because I'm not sure if log entries that
    big are not a flaw) */
    ld.data.builtin.count =
      std::numeric_limits<decltype (ld.data.builtin.count)>::max();
  }
  return ld;
}
//------------------------------------------------------------------------------
} // namespace malcpp {
