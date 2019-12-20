#include <memory>
#include <string>
#include <vector>
#include <limits>
#include <cassert>

#include <bl/base/platform.h>

#include <malcpp/malcpp.hpp>
#include <malcpp/custom_type.hpp>

namespace malcpp {
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
    ld = obj_log_data::get (*static_cast<std::string*> (ptr));
  }
  else {
    ld = obj_log_data::get_null_smartptr();
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
    ld = obj_log_data::get_as_str (header[obj->extra.flag], sizeof "u08[" - 1);
  }
  else {
    ld = obj_log_data::get ("[unknown vector]");
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
      ld = obj_log_data::get (vector_cast<bl_u8> (ptr));
      break;
    case malc_obj_u16:
      ld = obj_log_data::get (vector_cast<bl_u16> (ptr));
      break;
    case malc_obj_u32:
      ld = obj_log_data::get (vector_cast<bl_u32> (ptr));
      break;
    case malc_obj_u64:
      ld = obj_log_data::get (vector_cast<bl_u64> (ptr));
      break;
    case malc_obj_i8:
      ld = obj_log_data::get (vector_cast<bl_i8> (ptr));
      break;
    case malc_obj_i16:
      ld = obj_log_data::get (vector_cast<bl_i16> (ptr));
      break;
    case malc_obj_i32:
      ld = obj_log_data::get (vector_cast<bl_i32> (ptr));
      break;
    case malc_obj_i64:
      ld = obj_log_data::get (vector_cast<bl_i64> (ptr));
      break;
    case malc_obj_float:
      ld = obj_log_data::get (vector_cast<float> (ptr));
      break;
    case malc_obj_double:
      ld = obj_log_data::get (vector_cast<double> (ptr));
      break;
    default: /* unreachable, validated on the beggining*/
      break;
    }
  }
  else {
    ld = obj_log_data::get_null_smartptr();
  }
  err = malc_obj_push (pc, &ld);
  if (err) {
    return err;
  }
  //footer
  ld = obj_log_data::get ("]");
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
  BL_TRY {
    std::ostringstream ostream;
    bool notnull = tbl->print (obj->obj, ostream);
    if (notnull) {
      str = ostream.str();
      ld  = obj_log_data::get (str);
    }
    else {
      ld = obj_log_data::get_null_smartptr();
    }
  }
  BL_CATCH (...) {
    return 1;
  }
  return malc_obj_push (pc, &ld);
}
//------------------------------------------------------------------------------
} // namespace malcpp
