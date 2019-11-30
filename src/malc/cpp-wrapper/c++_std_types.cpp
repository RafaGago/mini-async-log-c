#include <memory>
#include <string>
#include <vector>

#include <malc/malc.hpp>

namespace malcpp {

//------------------------------------------------------------------------------
template <class T>
void shared_ptr_vector_fill_data (malc_obj_ref& obj, malc_obj_log_data& out)
{
  auto p = static_cast<std::shared_ptr<std::vector<T> > *> (obj.obj);
  if (auto vecptr = p->get()) {
    std::size_t size = vecptr->size();
    if (size == 0) {
      return;
    }
    auto bytes = size * sizeof (T);
    out.data.mem.ptr  = &(*vecptr)[0];
    out.data.mem.size = bytes;

    static_assert(
      sizeof (std::size_t) >= sizeof (decltype (out.data.mem.size)), ""
      );

    if (bytes < size || out.data.mem.size < bytes) {
      /* overflow: truncating */
      out.data.str.len = (decltype (out.data.str.len)) -1ull;
    }
  }
}
//------------------------------------------------------------------------------
MALC_EXPORT void string_shared_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context
  )
{
  if (!out) {
    /* no deallocations to be made */
    return;
  }
  auto ptr = static_cast<std::shared_ptr<std::string>*> (obj->obj);
  std::string* s = ptr->get();
  if (s) {
    out->is_str = 1;
    out->data.str.ptr = s->c_str();
    out->data.str.len = s->size();
    if (out->data.str.len < s->size()) {
      /* overflow: truncating */
      out->data.str.len = (decltype (out->data.str.len)) -1ull;
    }
  }
}
//------------------------------------------------------------------------------
MALC_EXPORT void integral_vector_shared_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context
  )
{
  if (!out) {
    /* no deallocations to be made */
    return;
  }
  static const char* header[] = {
    "u08[", "u16[", "u32[", "u64[", "s08[", "s16[", "s32[", "s64["
  };
  if (*iter_context == nullptr) {
    /* first call */
    if (obj->extra.flag > 7) {
      /* error: invalid integral type */
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
  case 0:
    shared_ptr_vector_fill_data<bl_u8> (*obj, *out);
    break;
  case 1:
    shared_ptr_vector_fill_data<bl_u16> (*obj, *out);
    break;
  case 2:
    shared_ptr_vector_fill_data<bl_u32> (*obj, *out);
    break;
  case 3:
    shared_ptr_vector_fill_data<bl_u64> (*obj, *out);
    break;
  case 4:
    shared_ptr_vector_fill_data<bl_i8> (*obj, *out);
    break;
  case 5:
    shared_ptr_vector_fill_data<bl_i16> (*obj, *out);
    break;
  case 6:
    shared_ptr_vector_fill_data<bl_i32> (*obj, *out);
    break;
  case 7:
    shared_ptr_vector_fill_data<bl_i64> (*obj, *out);
    break;
  default:
    break;
  }
  *iter_context = (void*) 2;
}
//------------------------------------------------------------------------------
} // namespace malcpp
