#include <memory>
#include <string>
#include <vector>
#include <limits>
#include <cassert>

#include <malc/malc.hpp>

namespace malcpp {

//------------------------------------------------------------------------------
static void fill_null_smart_ptr (malc_obj_log_data& out)
{
  out.is_str = 1;
  out.data.str.ptr = MALC_CPP_NULL_SMART_PTR_STR;
  out.data.str.len = sizeof MALC_CPP_NULL_SMART_PTR_STR - 1;
}
//------------------------------------------------------------------------------
MALC_EXPORT void string_smartptr_get_data(
  malc_obj_ref*       obj,
  malc_obj_log_data*  out,
  void**              iter_context,
  void const*         table,
  char const*,
  bl_alloc_tbl const*
  )
{
  if (!out) {
    /* no deallocations to be made */
    return;
  }
  assert (obj);
  auto tbl = static_cast<detail::serialization::smartptr_table const*> (table);
  if (void* ptr = tbl->dereference (obj->obj)) {
    std::string& s = *static_cast<std::string*> (ptr);
    out->is_str = 1;
    out->data.str.ptr = s.c_str();
    out->data.str.len = s.size();
    if (out->data.str.len < s.size()) {
      /* overflow: truncating */
      out->data.str.len =
        std::numeric_limits<decltype (out->data.str.len)>::max();
    }
  }
  else {
    fill_null_smart_ptr (*out);
  }
}
//------------------------------------------------------------------------------
template <class T>
static void vector_filler(
  std::vector<T>& v, malc_obj_log_data& out, bl_u8 builtin_type
  )
{
  std::size_t size = v.size();
  if (size == 0) {
    return;
  }
  out.data.builtin.ptr   = &v[0];
  out.data.builtin.count = size;
  out.data.builtin.type  = builtin_type;
  if (out.data.builtin.count < size) {
    /* overflow: truncating. (Notice that returning more values us p) */
    out.data.builtin.count =
      std::numeric_limits<decltype (out.data.builtin.count)>::max();
  }
}
//------------------------------------------------------------------------------
MALC_EXPORT void vector_smartptr_get_data(
  malc_obj_ref*       obj,
  malc_obj_log_data*  out,
  void**              iter_context,
  void const*         table,
  char const*,
  bl_alloc_tbl const*
  )
{
  if (!out) {
    /* no deallocations to be made */
    return;
  }
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
  if (*iter_context == nullptr) {
    /* first call */
    if (obj->extra.flag > malc_obj_double) {
      /* error: invalid type */
      return;
    }
    out->data.str.ptr = header[obj->extra.flag];
    out->data.str.len = sizeof "u08[" - 1;
    out->is_str = 1;
    *iter_context = (void*) 1;
    return;
  }
  if (2 == *((bl_uword*) iter_context)) {
    /* closing */
    out->data.str.ptr = "]";
    out->data.str.len = sizeof "]" - 1;
    out->is_str = 1;
    *iter_context = nullptr;
    return;
  }
  /* logging the vector data */
  *iter_context = (void*) 2;
  auto tbl = static_cast<detail::serialization::smartptr_table const*> (table);
  void* ptr = tbl->dereference (obj->obj);
  if (!ptr) {
    fill_null_smart_ptr (*out);
    return;
  }
  switch (obj->extra.flag) {
    /* notice: "extra.flag" could be passed through the table to save 1 byte,
    but it would require a different template specialization. This premature
    optimization is not done. */
  case malc_obj_u8:
    vector_filler (*((std::vector<bl_u8>*) ptr), *out, obj->extra.flag);
    break;
  case malc_obj_u16:
    vector_filler (*((std::vector<bl_u16>*) ptr), *out, obj->extra.flag);
    break;
  case malc_obj_u32:
    vector_filler (*((std::vector<bl_u32>*) ptr), *out, obj->extra.flag);
    break;
  case malc_obj_u64:
    vector_filler (*((std::vector<bl_u64>*) ptr), *out, obj->extra.flag);
    break;
  case malc_obj_i8:
    vector_filler (*((std::vector<bl_i8>*) ptr), *out, obj->extra.flag);
    break;
  case malc_obj_i16:
    vector_filler (*((std::vector<bl_i16>*) ptr), *out, obj->extra.flag);
    break;
  case malc_obj_i32:
    vector_filler (*((std::vector<bl_i32>*) ptr), *out, obj->extra.flag);
    break;
  case malc_obj_i64:
    vector_filler (*((std::vector<bl_i64>*) ptr), *out, obj->extra.flag);
    break;
  case malc_obj_float:
    vector_filler (*((std::vector<float>*) ptr), *out, obj->extra.flag);
    break;
  case malc_obj_double:
    vector_filler (*((std::vector<double>*) ptr), *out, obj->extra.flag);
    break;
  default:
    break;
  }
}
//------------------------------------------------------------------------------
MALC_EXPORT void ostringstream_get_data(
  malc_obj_ref*       obj,
  malc_obj_log_data*  out,
  void**              iter_context,
  void const*         table,
  char const*,
  bl_alloc_tbl const*
  )
{
  if (*iter_context) {
    /*2nd call*/
    delete (std::string*) *iter_context;
    *iter_context = nullptr;
    return;
  }
  /*1st call*/
  auto tbl = static_cast<
    detail::serialization::ostreamable_table const*
    > (table);
  std::string* str;
  try {
    std::ostringstream ostream;
    str = new std::string;
    bool notnull = tbl->print (obj->obj, ostream);
    if (notnull) {
      *str = ostream.str();
      out->data.str.ptr = str->c_str();
      out->data.str.len = str->size();
      out->is_str = 1;
    }
    else {
      fill_null_smart_ptr (*out);
    }
    *iter_context = (void*) str;
  }
  catch (...) {
    if (str) {
      out->data.str.ptr = nullptr;
      out->data.str.len = 0;
      delete str;
    }
  }
}
//------------------------------------------------------------------------------
} // namespace malcpp
