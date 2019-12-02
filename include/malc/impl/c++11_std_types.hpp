#ifndef __MALC_CPP_HPP__
#define __MALC_CPP_HPP__

#include <memory>
#include <cstddef>
#include <utility>
#include <type_traits>
#include <vector>

#ifndef MALC_COMMON_NAMESPACED
#define MALC_COMMON_NAMESPACED 1
#endif

#include <bl/base/static_integer_math.h>

#include <malc/impl/serialization.hpp>

// A header including C++ types. To avoid header bloat.

// Most of the types included here are wrapped on shared_ptr, as it is the only
// standard way to have object lifetime guarantees on C++

// NOTICE: As this is C++11, "std::shared_ptr<char const[]>" and
// "std::shared_ptr<void const[]>" are not provided, as they require a default
// deleter that class "delete[]" that the user is prone to miss.

#warning "TODO: weak_ptr"
#warning "TODO: mutex wrapping or example about how to do it"
#warning "TODO: wrapper for malc_obj_get_data_fn"
#warning "TODO: ostream adapters"

namespace malcpp {

extern MALC_EXPORT void string_shared_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context, char const* f
  );
extern MALC_EXPORT void vector_shared_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context, char const*
  );

namespace detail { namespace serialization {

/*----------------------------------------------------------------------------*/
/* interface types */
/*----------------------------------------------------------------------------*/
template <class T, template <class> class smartptr>
struct malc_obj_smartptr {
  malc_obj_get_data_fn getdata;
  malc_obj_destroy_fn  destroy;
  smartptr<T>          ptr;
};

template <class T, template <class> class smartptr>
struct malc_obj_smartptr_w_context : public malc_obj_smartptr<T, smartptr> {
  void* context;
};

template <class T, template <class> class smartptr>
struct malc_obj_smartptr_w_flag : public malc_obj_smartptr<T, smartptr> {
  bl_u8 flag;
};
/*----------------------------------------------------------------------------*/
/* serialization (transformed) types */
/*----------------------------------------------------------------------------*/
template <class T>
struct malc_transformed_obj {
#if MALC_PTR_COMPRESSION == 0
  malc_obj_get_data_fn getdata;
  malc_obj_destroy_fn  destroy;
#else
  malc_compressed_ptr  getdata;
  malc_compressed_ptr  destroy;
#endif
  typename std::aligned_storage<sizeof (T), alignof (T)>::type storage;
};

template <class T>
struct malc_transformed_obj_w_context {
  malc_transformed_obj<T> base;
#if MALC_PTR_COMPRESSION == 0
  void* context;
#else
  malc_compressed_ptr context;
#endif
};

template <class T>
struct malc_transformed_obj_w_flag {
  malc_transformed_obj<T> base;
  bl_u8                   flag;
};
/*----------------------------------------------------------------------------*/
/* type transformations. Classes of smart pointers to RAW
malc_transformed_obj_*
*/
/*----------------------------------------------------------------------------*/
template <class T, template <class> class smartptr>
struct type<malc_obj_smartptr<T, smartptr> > {

  static_assert(
    sizeof (smartptr<T>) <= 255,
    "malc can only serialize small objects from 0 to 255 bytes."
    );
  static_assert(
    alignof (smartptr<T>) <= MALC_OBJ_MAX_ALIGN,
    "malc can only serialize objects aligned up to \"std::max_align_t\"."
    );

  using transformed = malc_transformed_obj<smartptr<T> >;

  static constexpr char id = malc_type_obj;

  static inline transformed transform (malc_obj_smartptr<T, smartptr>& v)
  {
    transformed r;
    new (&r.storage) smartptr<T> (std::move (v.ptr));
#if MALC_PTR_COMPRESSION == 0
    r.getdata = v.getdata;
    r.destroy = v.destroy;
#else
    r.getdata = malc_get_compressed_ptr (v.getdata);
    r.destroy = malc_get_compressed_ptr (v.destroy);
#endif
    return r;
  }
};
/*----------------------------------------------------------------------------*/
template <class T, template <class> class smartptr>
struct type<malc_obj_smartptr_w_context<T, smartptr> > {

  using base = type<malc_obj_smartptr<T, smartptr> >;
  using transformed = malc_transformed_obj_w_context<smartptr<T> >;

  static constexpr char id = malc_type_obj_ctx;

  static inline transformed transform(
    malc_obj_smartptr_w_context<T, smartptr>& v
    )
  {
    transformed r;
    r.base = base::transform (static_cast<malc_obj_smartptr<T, smartptr>&> (v));
#if MALC_PTR_COMPRESSION == 0
    r.context = v.context;
#else
    r.context = malc_get_compressed_ptr (v.context);
#endif
    return r;
  }
};
/*----------------------------------------------------------------------------*/
template <class T, template <class> class smartptr>
struct type<malc_obj_smartptr_w_flag<T, smartptr> > {

  using base = type<malc_obj_smartptr<T, smartptr> >;
  using transformed = malc_transformed_obj_w_flag<smartptr<T> >;

  static constexpr char id = malc_type_obj_flag;

  static inline transformed transform (malc_obj_smartptr_w_flag<T, smartptr>& v)
  {
    transformed r;
    r.base = base::transform (static_cast<malc_obj_smartptr<T, smartptr>&> (v));
    r.flag = v.flag;
    return r;
  }
};
/*----------------------------------------------------------------------------*/
/* Transformations for smart pointers, base classes. They take a smart pointer
and return malc_obj_smartptr_*
*/
/*----------------------------------------------------------------------------*/
template<malc_obj_get_data_fn getdata, class T, template <class> class smartptr>
struct smartptr_type  {

  using base = type<malc_obj_smartptr<T, smartptr> >;
  static constexpr char id = base::id;

  static void destroy (malc_obj_ref* obj)
  {
    using smartptrtype = smartptr<T>;
    static_cast<smartptrtype*> (obj->obj)->~smartptrtype();
  }

  static inline typename base::transformed transform (smartptr<T>& v)
  {
    malc_obj_smartptr<T, smartptr> b;
    b.ptr = v;
    b.getdata = getdata;
    b.destroy = destroy;
    return base::transform (b);
  }

  static inline typename base::transformed transform (smartptr<T>&& v)
  {
    malc_obj_smartptr<T, smartptr> b;
    b.ptr = std::move (v);
    b.getdata = getdata;
    b.destroy = destroy;
    return base::transform (b);
  }
};
/*----------------------------------------------------------------------------*/
template<
  malc_obj_get_data_fn   getdata,
  unsigned char          flag_default,
  class                  T,
  template <class> class smartptr
  >
struct smartptr_type_w_flag {
  using base = type<malc_obj_smartptr_w_flag<T, smartptr> >;
  static constexpr char id = base::id;

  static void destroy (malc_obj_ref* obj)
  {
    using smartptrtype = smartptr<T>;
    static_cast<smartptrtype*> (obj->obj)->~smartptrtype();
  }

  static inline typename base::transformed transform(
    smartptr<T>& v, unsigned char flag = flag_default
    )
  {
    malc_obj_smartptr_w_flag<T, smartptr> b;
    b.ptr = v;
    b.getdata = getdata;
    b.destroy = destroy;
    b.flag    = flag;
    return base::transform (b);
  }

  static inline typename base::transformed transform(
    smartptr<T>&& v, unsigned char flag = flag_default
    )
  {
    malc_obj_smartptr_w_flag<T, smartptr> b;
    b.ptr = std::move (v);
    b.getdata = getdata;
    b.destroy = destroy;
    b.flag    = flag;
    return base::transform (b);
  }
};
/*----------------------------------------------------------------------------*/
/* type instantiations of smart pointer types*/
/*----------------------------------------------------------------------------*/
template<>
struct type<std::shared_ptr<std::string>> :
  public smartptr_type<
    string_shared_ptr_get_data, std::string, std::shared_ptr
    >
{};
/*----------------------------------------------------------------------------*/
template <class T>
static inline constexpr bl_u8 encode_int_type_flag()
{
  return (bl_static_log2_ceil_u8 (sizeof (T))) |
    (std::is_signed<T>::value ? (1 << 2) : 0);
}
/*----------------------------------------------------------------------------*/
static_assert (encode_int_type_flag<bl_u8>()  == malc_obj_u8, "");
static_assert (encode_int_type_flag<bl_u16>() == malc_obj_u16, "");
static_assert (encode_int_type_flag<bl_u32>() == malc_obj_u32, "");
static_assert (encode_int_type_flag<bl_u64>() == malc_obj_u64, "");
static_assert (encode_int_type_flag<bl_i8>()  == malc_obj_i8, "");
static_assert (encode_int_type_flag<bl_i16>() == malc_obj_i16, "");
static_assert (encode_int_type_flag<bl_i32>() == malc_obj_i32, "");
static_assert (encode_int_type_flag<bl_i64>() == malc_obj_i64, "");
/*----------------------------------------------------------------------------*/
template <class T, class... Args>
struct type<
  std::shared_ptr<std::vector<T, Args...> > ,
  typename std::enable_if<
    std::is_integral<T>::value && !std::is_same<T, bool>::value
    >::type
  > :
  public smartptr_type_w_flag<
    vector_shared_ptr_get_data,
    encode_int_type_flag<T>(),
    std::vector<T, Args...>,
    std::shared_ptr
    >
{};
/*----------------------------------------------------------------------------*/
template <class T, class... Args>
struct type<
  std::shared_ptr<std::vector<T, Args...> > ,
  typename std::enable_if<
    std::is_floating_point<T>::value
    >::type
  > :
  public smartptr_type_w_flag<
    vector_shared_ptr_get_data,
    std::is_same<T, float>::value ? malc_obj_float : malc_obj_double,
    std::vector<T, Args...>,
    std::shared_ptr
    >
{};
/*----------------------------------------------------------------------------*/
/* type size bases. After calling transform there always be
malc_transformed_obj* types. We have to provide its sizes. These are base clases
to inherit by fully typed transformations.
*/
/*----------------------------------------------------------------------------*/
template <class T, template <class> class smartptr>
struct size_type_base {
  using transformed = malc_transformed_obj<smartptr<T> >;

  static inline bl_uword size (const transformed& v)
  {
    return
#if MALC_PTR_COMPRESSION == 0
      bl_sizeof_member (transformed, getdata)
      + bl_sizeof_member (transformed, destroy)
#else
      malc_compressed_get_size (v.getdata.format_nibble)
      + malc_compressed_get_size (v.destroy.format_nibble)
#endif
      + sizeof (bl_u8) // object size field
      + sizeof (smartptr<T>);
  }
};
/*----------------------------------------------------------------------------*/
template <class T, template <class> class smartptr>
struct size_type_w_context_base {
  using base = type<malc_transformed_obj<smartptr<T> > >;
  using transformed = malc_transformed_obj_w_context<smartptr<T> >;

  static inline bl_uword size (const transformed& v)
  {
    return base::size (v.base)
#if MALC_PTR_COMPRESSION == 0
      + bl_sizeof_member (transformed, context);
#else
      + malc_compressed_get_size (v.context.format_nibble);
#endif
  }
};
/*----------------------------------------------------------------------------*/
template <class T, template <class> class smartptr>
struct size_type_w_flag_base {
  using base = type<malc_transformed_obj<smartptr<T> > >;
  using transformed = malc_transformed_obj_w_flag<smartptr<T> >;

  static inline bl_uword size (const transformed& v)
  {
    return base::size (v.base)
      + bl_sizeof_member (transformed, flag);
  }
};
/*----------------------------------------------------------------------------*/
/* Providing real instantiations for size calculations */
/*----------------------------------------------------------------------------*/
template <class T>
struct type<malc_transformed_obj<std::shared_ptr<T> > > :
  public size_type_base<T, std::shared_ptr>
{};
/*----------------------------------------------------------------------------*/
template <class T>
struct type<malc_transformed_obj_w_context<std::shared_ptr<T> > > :
  public size_type_w_context_base<T, std::shared_ptr>
{};
/*----------------------------------------------------------------------------*/
template <class T>
struct type<malc_transformed_obj_w_flag<std::shared_ptr<T> > > :
  public size_type_w_flag_base<T, std::shared_ptr>
{};
/*----------------------------------------------------------------------------*/
/* serializations */
/*----------------------------------------------------------------------------*/
template <class T>
static inline void serialize_type (malc_serializer& s, T& v)
{
  memcpy (s.field_mem, &v, sizeof v);
  s.field_mem += sizeof v;
};
/*----------------------------------------------------------------------------*/
#if MALC_PTR_COMPRESSION == 1
static inline void serialize_type (malc_serializer& s, malc_compressed_ptr& v)
{
  malc_serialize_compressed_ptr (&s, v);
};
#endif
/*----------------------------------------------------------------------------*/
template <class T>
static inline void malc_serialize(
  malc_serializer* s, const malc_transformed_obj<T>& v
  )
{
  serialize_type (*s, v.getdata);
  serialize_type (*s, v.destroy);
  static constexpr bl_u8 size = sizeof (T);
  serialize_type (*s, size);
  memcpy (s->field_mem, &v.storage, size);
  s->field_mem += size;
}
/*----------------------------------------------------------------------------*/
template <class T>
static inline void malc_serialize(
  malc_serializer* s, const malc_transformed_obj_w_context<T>& v
  )
{
  serialize_type (*s, v.context);
  malc_serialize (s, v.base);
}
/*----------------------------------------------------------------------------*/
template <class T>
static inline void malc_serialize(
  malc_serializer* s, const malc_transformed_obj_w_flag<T>& v
  )
{
  serialize_type (*s, v.flag);
  malc_serialize (s, v.base);
}
/*----------------------------------------------------------------------------*/
}}} // namespace malcpp { namespace detail { namespace serialization {

#endif // __MALC_CPP_STD_TYPES_HPP__
