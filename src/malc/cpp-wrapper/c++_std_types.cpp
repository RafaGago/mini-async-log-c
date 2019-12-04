#include <memory>
#include <string>
#include <vector>
#include <limits>
#include <cassert>

#include <malc/malc.hpp>

namespace malcpp {

//------------------------------------------------------------------------------
static malc_obj_log_data fill_logdata (char const* str, std::size_t len)
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
static inline malc_obj_log_data fill_logdata (std::string const& s)
{
  return fill_logdata (s.c_str(), s.size());
}
//------------------------------------------------------------------------------
static malc_obj_log_data fill_logdata(
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
template <class T>
static inline malc_obj_log_data fill_logdata(
  std::vector<T> const &v, bl_u8 builtin_type
  )
{
  return fill_logdata ((void*) &v[0], v.size(), builtin_type);
}
//------------------------------------------------------------------------------
#define fill_logdata_lit(lit) fill_logdata (lit, sizeof lit - 1)
//------------------------------------------------------------------------------
static inline malc_obj_log_data fill_logdata_null_smartptr()
{
  return fill_logdata_lit (MALC_CPP_NULL_SMART_PTR_STR);
}
//------------------------------------------------------------------------------
MALC_EXPORT int string_smartptr_get_data(
  malc_obj_ref*                obj,
  void const*                  table,
  malc_obj_push_context const* pc,
  bl_alloc_tbl const*
  )
{
  malc_obj_log_data ld;
  auto tbl = static_cast<detail::serialization::smartptr_table const*> (table);
  if (void* ptr = tbl->dereference (obj->obj)) {
    ld = fill_logdata (*static_cast<std::string*> (ptr));
  }
  else {
    ld = fill_logdata_null_smartptr();
  }
  return malc_obj_push (pc, &ld);
}
//------------------------------------------------------------------------------
template <class T>
static inline std::vector<T> const& vector_cast (void* ptr)
{
  return *static_cast<std::vector<T> const*> (ptr);
}
//------------------------------------------------------------------------------
MALC_EXPORT int vector_smartptr_get_data(
  malc_obj_ref*                obj,
  void const*                  table,
  malc_obj_push_context const* pc,
  bl_alloc_tbl const*
  )
{
  static const char* header[] = {
    "u08[",
    "u16[",
    "u32[",
    "u64[",
    "s08[",
    "s16[",
    "s32[",
    "s64[",
    "flt[",
    "dbl["
  };
  malc_obj_log_data ld;
  //header
  if (obj->extra.flag <= malc_obj_double) {
    ld = fill_logdata (header[obj->extra.flag], sizeof "u08[" - 1);
  }
  else {
    ld = fill_logdata_lit ("[unknown vector]");
  }
  int err = malc_obj_push (pc, &ld);
  if (err) {
    return err;
  }
  //log type
  auto tbl = static_cast<detail::serialization::smartptr_table const*> (table);
  void* ptr = tbl->dereference (obj->obj);
  if (ptr) {
    switch (obj->extra.flag) {
      /* notice: "extra.flag" could be passed through the table to save 1 byte,
      but it would require a different template specialization. This premature
      optimization is not done. */
    case malc_obj_u8:
      ld = fill_logdata (vector_cast<bl_u8> (ptr), obj->extra.flag);
      break;
    case malc_obj_u16:
      ld = fill_logdata (vector_cast<bl_u16> (ptr), obj->extra.flag);
      break;
    case malc_obj_u32:
      ld = fill_logdata (vector_cast<bl_u32> (ptr), obj->extra.flag);
      break;
    case malc_obj_u64:
      ld = fill_logdata (vector_cast<bl_u64> (ptr), obj->extra.flag);
      break;
    case malc_obj_i8:
      ld = fill_logdata (vector_cast<bl_i8> (ptr), obj->extra.flag);
      break;
    case malc_obj_i16:
      ld = fill_logdata (vector_cast<bl_i16> (ptr), obj->extra.flag);
      break;
    case malc_obj_i32:
      ld = fill_logdata (vector_cast<bl_i32> (ptr), obj->extra.flag);
      break;
    case malc_obj_i64:
      ld = fill_logdata (vector_cast<bl_i64> (ptr), obj->extra.flag);
      break;
    case malc_obj_float:
      ld = fill_logdata (vector_cast<float> (ptr), obj->extra.flag);
      break;
    case malc_obj_double:
      ld = fill_logdata (vector_cast<double> (ptr), obj->extra.flag);
      break;
    default: /* unreachable, validated on the beggining*/
      break;
    }
  }
  else {
    ld = fill_logdata_null_smartptr();
  }
  err = malc_obj_push (pc, &ld);
  if (err) {
    return err;
  }
  //footer
  ld = fill_logdata_lit ("]");
  return malc_obj_push (pc, &ld);
}
//------------------------------------------------------------------------------
MALC_EXPORT int ostringstream_get_data(
  malc_obj_ref*                obj,
  void const*                  table,
  malc_obj_push_context const* pc,
  bl_alloc_tbl const*
  )
{
  auto tbl =
  static_cast<detail::serialization::ostreamable_table const*> (table);
  malc_obj_log_data ld;
  std::string str;
  try {
    std::ostringstream ostream;
    bool notnull = tbl->print (obj->obj, ostream);
    if (notnull) {
      str = ostream.str();
      ld  = fill_logdata (str);
    }
    else {
      ld = fill_logdata_null_smartptr();
    }
  }
  catch (...) {
    return 1;
  }
  return malc_obj_push (pc, &ld);
}
//------------------------------------------------------------------------------
} // namespace malcpp
