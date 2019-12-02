#include <memory>
#include <string>
#include <vector>
#include <limits>
#include <cassert>

#include <malc/malc.hpp>

namespace malcpp {

//------------------------------------------------------------------------------
static void string_filler (std::string& s, malc_obj_log_data& out)
{
  out.is_str = 1;
  out.data.str.ptr = s.c_str();
  out.data.str.len = s.size();
  if (out.data.str.len < s.size()) {
    /* overflow: truncating */
    out.data.str.len =
      std::numeric_limits<decltype (out.data.str.len)>::max();
  }
}
//------------------------------------------------------------------------------
template <class T>
static T* shared_ptr_try_get (malc_obj_ref& obj)
{
  auto p = static_cast<std::shared_ptr<T>*> (obj.obj);
  return p->get();
}
//------------------------------------------------------------------------------
template <class T>
static T* weak_ptr_try_get (malc_obj_ref& obj, malc_obj_log_data& out)
{
  auto w = static_cast<std::weak_ptr<T>*> (obj.obj);
  if (auto p = w->lock()) {
    return p.get();
  }
  else {
    out.is_str = 1;
    out.data.str.ptr = MALC_CPP_NULL_WEAK_PTR_STR;
    out.data.str.len = sizeof MALC_CPP_NULL_WEAK_PTR_STR - 1;
    return nullptr;
  }
}
//------------------------------------------------------------------------------
template <class T>
static T* unique_ptr_try_get (malc_obj_ref& obj)
{
  auto p = static_cast<std::unique_ptr<T>*> (obj.obj);
  return p->get();
}
//------------------------------------------------------------------------------
MALC_EXPORT void string_shared_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context, char const*
  )
{
  if (!out) {
    /* no deallocations to be made */
    return;
  }
  assert (obj);
  if (auto s = shared_ptr_try_get<std::string> (*obj)) {
    string_filler (*s, *out);
  }
}
//------------------------------------------------------------------------------
MALC_EXPORT void string_weak_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context, char const*
  )
{
  if (!out) {
    /* no deallocations to be made */
    return;
  }
  assert (obj);
  if (auto s = weak_ptr_try_get<std::string> (*obj, *out)) {
    string_filler (*s, *out);
  }
}
//------------------------------------------------------------------------------
MALC_EXPORT void string_unique_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context, char const*
  )
{
  if (!out) {
    /* no deallocations to be made */
    return;
  }
  assert (obj);
  if (auto s = unique_ptr_try_get<std::string> (*obj)) {
    string_filler (*s, *out);
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
template <class T>
struct shared_ptr_vector_filler {
  static void run(
    malc_obj_ref& obj, malc_obj_log_data& out, bl_u8 builtin_type
    )
  {
    if (auto vecptr = shared_ptr_try_get<std::vector<T>> (obj)) {
      vector_filler (*vecptr, out, builtin_type);
    }
  }
};
//------------------------------------------------------------------------------
template <class T>
struct weak_ptr_vector_filler {
  static void run(
    malc_obj_ref& obj, malc_obj_log_data& out, bl_u8 builtin_type
    )
  {
    if (auto vecptr = weak_ptr_try_get<std::vector<T> > (obj, out)) {
      vector_filler (*vecptr, out, builtin_type);
    }
  }
};
//------------------------------------------------------------------------------
template <class T>
struct unique_ptr_vector_filler {
  static void run(
    malc_obj_ref& obj, malc_obj_log_data& out, bl_u8 builtin_type
    )
  {
    if (auto vecptr = unique_ptr_try_get<std::vector<T> > (obj)) {
      vector_filler (*vecptr, out, builtin_type);
    }
  }
};
//------------------------------------------------------------------------------
template <template <class> class filler >
static inline void vector_smartptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context
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
  switch (obj->extra.flag) {
  case malc_obj_u8:
    filler<bl_u8>::run (*obj, *out, obj->extra.flag);
    break;
  case malc_obj_u16:
    filler<bl_u16>::run (*obj, *out, obj->extra.flag);
    break;
  case malc_obj_u32:
    filler<bl_u32>::run (*obj, *out, obj->extra.flag);
    break;
  case malc_obj_u64:
    filler<bl_u64>::run (*obj, *out, obj->extra.flag);
    break;
  case malc_obj_i8:
    filler<bl_i8>::run (*obj, *out, obj->extra.flag);
    break;
  case malc_obj_i16:
    filler<bl_i16>::run (*obj, *out, obj->extra.flag);
    break;
  case malc_obj_i32:
    filler<bl_i32>::run (*obj, *out, obj->extra.flag);
    break;
  case malc_obj_i64:
    filler<bl_i64>::run (*obj, *out, obj->extra.flag);
    break;
  case malc_obj_float:
    filler<float>::run (*obj, *out, obj->extra.flag);
    break;
  case malc_obj_double:
    filler<double>::run (*obj, *out, obj->extra.flag);
    break;
  default:
    break;
  }
  *iter_context = (void*) 2;
}
//------------------------------------------------------------------------------
MALC_EXPORT void vector_shared_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context, char const*
  )
{
  vector_smartptr_get_data<shared_ptr_vector_filler> (obj, out, iter_context);
}
//------------------------------------------------------------------------------
MALC_EXPORT void vector_weak_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context, char const*
  )
{
  vector_smartptr_get_data<weak_ptr_vector_filler> (obj, out, iter_context);
}
//------------------------------------------------------------------------------
MALC_EXPORT void vector_unique_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context, char const*
  )
{
  vector_smartptr_get_data<unique_ptr_vector_filler> (obj, out, iter_context);
}
//------------------------------------------------------------------------------
} // namespace malcpp
