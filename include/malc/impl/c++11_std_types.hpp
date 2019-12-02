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

#warning "TODO: mutex wrapping or example about how to do it"
#warning "TODO: ostream adapters"
#warning "TODO: Move the serialization and definitions to the C header"
#warning "TODO: allow obj types from C"
#warning "TODO: examples obj types from C"
#warning "TODO: examples obj types from C++"
#warning "TODO: wrapper for writing get_data_fn"
#warning "TODO: move "get_data" implementations to separate translation units"

#ifndef MALC_CPP_NULL_WEAK_PTR_STR
  #define MALC_CPP_NULL_WEAK_PTR_STR "null_weak_ptr"
#endif

namespace malcpp {

extern MALC_EXPORT void string_shared_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context, char const* f
  );
extern MALC_EXPORT void vector_shared_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context, char const* f
  );
extern MALC_EXPORT void string_weak_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context, char const* f
  );
extern MALC_EXPORT void vector_weak_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context, char const* f
  );
extern MALC_EXPORT void string_unique_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context, char const* f
  );
extern MALC_EXPORT void vector_unique_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context, char const* f
  );

namespace detail { namespace serialization {

template <class T, template <class, class...> class smartptr>
struct get_data_function;

template <>
struct get_data_function<std::string, std::shared_ptr> {
  static constexpr malc_obj_get_data_fn value = string_shared_ptr_get_data;
};

template <class T, class... types>
struct get_data_function<std::vector<T, types...>, std::shared_ptr> {
  static constexpr malc_obj_get_data_fn value = vector_shared_ptr_get_data;
};

template <>
struct get_data_function<std::string, std::weak_ptr> {
  static constexpr malc_obj_get_data_fn value = string_weak_ptr_get_data;
};

template <class T, class... types>
struct get_data_function<std::vector<T, types...>, std::weak_ptr> {
  static constexpr malc_obj_get_data_fn value = vector_weak_ptr_get_data;
};

template <>
struct get_data_function<std::string, std::unique_ptr> {
  static constexpr malc_obj_get_data_fn value = string_unique_ptr_get_data;
};

template <class T, class... types>
struct get_data_function<std::vector<T, types...>, std::unique_ptr> {
  static constexpr malc_obj_get_data_fn value = vector_unique_ptr_get_data;
};

/*----------------------------------------------------------------------------*/
/* interface types */
/*----------------------------------------------------------------------------*/
template <class T, template <class, class...> class smartptr, class... types>
struct malc_obj_smartptr {
  malc_obj_get_data_fn  getdata;
  malc_obj_destroy_fn   destroy;
  smartptr<T, types...> ptr;
};

template <class T, template <class, class...> class smartptr, class... types>
struct malc_obj_smartptr_w_context :
    public malc_obj_smartptr<T, smartptr, types...> {
  void* context;
};

template <class T, template <class, class...> class smartptr, class... types>
struct malc_obj_smartptr_w_flag :
    public malc_obj_smartptr<T, smartptr, types...> {
  bl_u8 flag;
};
/*----------------------------------------------------------------------------*/
/* serialization types */
/*----------------------------------------------------------------------------*/
template <class T>
struct malc_serializable_obj {
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
struct malc_serializable_obj_w_context {
  malc_serializable_obj<T> base;
#if MALC_PTR_COMPRESSION == 0
  void* context;
#else
  malc_compressed_ptr context;
#endif
};

template <class T>
struct malc_serializable_obj_w_flag {
  malc_serializable_obj<T> base;
  bl_u8                    flag;
};
/*----------------------------------------------------------------------------*/
/* helper classes to avoid invalid instantiations for non-copyable but movable
objects. */
/*----------------------------------------------------------------------------*/
template <bool move>
struct move_helper {
  template <class T>
  static inline T&& run (T& v)
  {
    return std::move (v);
  }
};

template <>
struct move_helper<false> {
  template <class T>
  static inline T& run (T& v)
  {
    return v;
  }
};
/*----------------------------------------------------------------------------*/
/* type transformations. It is possible to log these (malc_serializable_obj_*)
types directly, but it requires the user to provide the "malc_obj_get_data_fn"
and "malc_obj_destroy_fn" functions manually. */
/*----------------------------------------------------------------------------*/
template <class T, template <class, class...> class smartptr, class... types>
struct sertype<malc_obj_smartptr<T, smartptr, types...> > {
public:
  static_assert(
    sizeof (smartptr<T, types...>) <= 255,
    "malc can only serialize small objects from 0 to 255 bytes."
    );
  static_assert(
    alignof (smartptr<T, types...>) <= MALC_OBJ_MAX_ALIGN,
    "malc can only serialize objects aligned up to \"std::max_align_t\"."
    );

  using serializable = malc_serializable_obj<smartptr<T, types...> >;
  using input_type   = malc_obj_smartptr<T, smartptr, types...>;

  static constexpr char id = malc_type_obj;

  static inline serializable to_serialization_type (input_type&& v)
  {
    return to_serialization_type_impl<true> (std::move (v));
  }

  static inline serializable to_serialization_type (input_type &v)
  {
    return to_serialization_type_impl<false> (v);
  }
private:
  template <bool move, class I>
  static inline serializable to_serialization_type_impl (I&& v)
  {
    serializable r;
    new (&r.storage) (decltype (v.ptr)) (move_helper<move>::run (v.ptr));
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
template <class T, template <class, class...> class smartptr, class... types>
struct sertype<malc_obj_smartptr_w_context<T, smartptr, types...> > {
public:
  using base = sertype<malc_obj_smartptr<T, smartptr, types...> >;
  using serializable = malc_serializable_obj_w_context<smartptr<T, types...> >;

  static constexpr char id = malc_type_obj_ctx;

  static inline serializable to_serialization_type(
    malc_obj_smartptr_w_context<T, smartptr, types...>&& v
    )
  {
    return to_serialization_type_impl<true> (std::move (v));
  }

  static inline serializable to_serialization_type(
    malc_obj_smartptr_w_context<T, smartptr, types...>& v
    )
  {
    return to_serialization_type_impl<false> (v);
  }

private:
  template <bool move, class I>
  static inline serializable to_serialization_type_impl (I&& v)
  {
    using pass    = malc_obj_smartptr<T, smartptr, types...>;
    using passref = typename std::conditional<move, pass&&, pass&>::type;

    serializable r;
    r.base = base::to_serialization_type(
        static_cast<passref> (move_helper<move>::run (v))
        );
#if MALC_PTR_COMPRESSION == 0
    r.context = v.context;
#else
    r.context = malc_get_compressed_ptr (v.context);
#endif
    return r;
  }
};
/*----------------------------------------------------------------------------*/
template <class T, template <class, class...> class smartptr, class... types>
struct sertype<malc_obj_smartptr_w_flag<T, smartptr, types...> > {
public:
  using base = sertype<malc_obj_smartptr<T, smartptr, types...> >;
  using serializable = malc_serializable_obj_w_flag<smartptr<T, types...> >;

  static constexpr char id = malc_type_obj_flag;

  static inline serializable
    to_serialization_type (malc_obj_smartptr_w_flag<T, smartptr, types...>&& v)
  {
    return to_serialization_type_impl<true> (std::move (v));
  }

  static inline serializable
    to_serialization_type (malc_obj_smartptr_w_flag<T, smartptr, types...>& v)
  {
    return to_serialization_type_impl<false> (v);
  }

private:
  template <bool move, class I>
  static inline serializable to_serialization_type_impl (I&& v)
  {
    using pass    = malc_obj_smartptr<T, smartptr, types...>;
    using passref = typename std::conditional<move, pass&&, pass&>::type;

    serializable r;
    r.base = base::to_serialization_type(
        static_cast<passref> (move_helper<move>::run (v))
        );
    r.flag = v.flag;
    return r;
  }
};
/*----------------------------------------------------------------------------*/
//providing "type" for serializable C++ types. (See comment above).
/*----------------------------------------------------------------------------*/
template <class T, template <class, class...> class smartptr, class... types>
struct sertype<malc_serializable_obj<smartptr<T, types...> > > {

  using serializable = malc_serializable_obj<smartptr<T, types...> >;

  static inline bl_uword size (serializable const& v)
  {
    return
#if MALC_PTR_COMPRESSION == 0
      bl_sizeof_member (serializable, getdata)
      + bl_sizeof_member (serializable, destroy)
#else
      malc_compressed_get_size (v.getdata.format_nibble)
      + malc_compressed_get_size (v.destroy.format_nibble)
#endif
      + sizeof (bl_u8) // object size field
      + sizeof (smartptr<T>);
  }
};
/*----------------------------------------------------------------------------*/
template <class T, template <class, class...> class smartptr, class... types>
struct sertype<malc_serializable_obj_w_context<smartptr<T, types...> > > {

  using base = sertype<malc_serializable_obj<smartptr<T, types...> > >;
  using serializable = malc_serializable_obj_w_context<smartptr<T, types...> >;

  static inline bl_uword size (serializable const& v)
  {
    return base::size (v.base)
#if MALC_PTR_COMPRESSION == 0
      + bl_sizeof_member (serializable, context);
#else
      + malc_compressed_get_size (v.context.format_nibble);
#endif
  }
};
/*----------------------------------------------------------------------------*/
template <class T, template <class, class...> class smartptr, class... types>
struct sertype<malc_serializable_obj_w_flag<smartptr<T, types...> > > {

  using base = sertype<malc_serializable_obj<smartptr<T, types...> > >;
  using serializable = malc_serializable_obj_w_flag<smartptr<T, types...> >;

  static inline bl_uword size (serializable const& v)
  {
    return base::size (v.base)
      + bl_sizeof_member (serializable, flag);
  }
};
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
  malc_serializer* s, malc_serializable_obj<T> const& v
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
  malc_serializer* s, malc_serializable_obj_w_context<T> const& v
  )
{
  serialize_type (*s, v.context);
  malc_serialize (s, v.base);
}
/*----------------------------------------------------------------------------*/
template <class T>
static inline void malc_serialize(
  malc_serializer* s, malc_serializable_obj_w_flag<T> const& v
  )
{
  serialize_type (*s, v.flag);
  malc_serialize (s, v.base);
}
/*----------------------------------------------------------------------------*/
/* validation helpers */
/*----------------------------------------------------------------------------*/
template <template <class, class...> class smartptr>
struct is_valid_smartptr : public std::conditional<
    std::is_same<smartptr<char>, std::shared_ptr<char> >::value ||
    std::is_same<smartptr<char>, std::weak_ptr<char> >::value ||
    std::is_same<smartptr<char>, std::unique_ptr<char> >::value,
    std::true_type,
    std::false_type
  >::type {};

template <template <class, class...> class container>
struct is_valid_container : public std::conditional<
    std::is_same<container<char>, std::vector<char> >::value,
    std::true_type,
    std::false_type
  >::type {};

template <class T>
struct is_valid_builtin : public std::conditional<
    std::is_arithmetic<T>::value && ! std::is_same<T, bool>::value,
    std::true_type,
    std::false_type
  >::type {};
/*----------------------------------------------------------------------------*/
/* Transformations for smart pointers. These create a serialization-friendly
type directly from C++ types.
*/
/*----------------------------------------------------------------------------*/
template<class T, template <class, class...> class smartptr, class... types>
struct smartptr_type  {
  using base = sertype<malc_obj_smartptr<T, smartptr, types...> >;
  static constexpr char id = base::id;

  static void destroy (malc_obj_ref* obj)
  {
    using smartptrtype = smartptr<T, types...>;
    static_cast<smartptrtype*> (obj->obj)->~smartptrtype();
  }

  static inline malc_obj_smartptr<T, smartptr, types...>
    to_obj_smartptr (smartptr<T, types...>& v)
  {
    malc_obj_smartptr<T, smartptr, types...> b;
    b.ptr = v;
    b.getdata = get_data_function<T, smartptr>::value;
    b.destroy = destroy;
    return b;
  }

  static inline malc_obj_smartptr<T, smartptr, types...>
    to_obj_smartptr (smartptr<T, types...>&& v)
  {
    malc_obj_smartptr<T, smartptr, types...> b;
    b.ptr = std::move (v);
    b.getdata = get_data_function<T, smartptr>::value;
    b.destroy = destroy;
    return b;
  }

  static inline typename base::serializable
    to_serialization_type (smartptr<T>& v)
  {
    return base::to_serialization_type (to_obj_smartptr (v));
  }

  static inline typename base::serializable
    to_serialization_type (smartptr<T>&& v)
  {
    return base::to_serialization_type (to_obj_smartptr (std::move (v)));
  }
};
/*----------------------------------------------------------------------------*/
template<
  class                            T,
  template <class, class...> class smartptr,
  unsigned char                    flag_default,
  class...                         types
  >
struct smartptr_type_w_flag {
  using base = sertype<malc_obj_smartptr_w_flag<T, smartptr, types...> >;
  static constexpr char id = base::id;

  static inline typename base::serializable to_serialization_type(
    smartptr<T>& v, unsigned char flag = flag_default
    )
  {
    malc_obj_smartptr_w_flag<T, smartptr, types...> b;
    *(static_cast<malc_obj_smartptr<T, smartptr, types...>*> (&b)) =
      smartptr_type<T, smartptr, types...>::to_obj_smartptr (v);
    b.flag = flag;
    return base::to_serialization_type (b);
  }

  static inline typename base::serializable to_serialization_type(
    smartptr<T, types...>&& v, unsigned char flag = flag_default
    )
  {
    malc_obj_smartptr_w_flag<T, smartptr, types...> b;
    *(static_cast<malc_obj_smartptr<T, smartptr, types...>*> (&b)) =
      smartptr_type<T, smartptr, types...>::to_obj_smartptr (std::move (v));
    b.flag = flag;
    return base::to_serialization_type (std::move (b));
  }
};
/*----------------------------------------------------------------------------*/
/* type instantiations of smart pointer types*/
/*----------------------------------------------------------------------------*/
template<template <class, class...> class smartptr, class... types>
struct sertype<
  smartptr<std::string, types...>,
  typename std::enable_if<is_valid_smartptr<smartptr>::value>::type
  > :
  public smartptr_type<std::string, smartptr, types...>
{};
/*----------------------------------------------------------------------------*/
template <class T>
struct arithmetic_type_flag {
  static constexpr bl_u8 get()
  {
    return (bl_static_log2_ceil_u8 (sizeof (T))) |
      (std::is_signed<T>::value ? (1 << 2) : 0);
  }
};

template <>
struct arithmetic_type_flag<float> {
  static constexpr bl_u8 get() { return malc_obj_float; }
};

template <>
struct arithmetic_type_flag<double> {
  static constexpr bl_u8 get() { return malc_obj_double; }
};
/*----------------------------------------------------------------------------*/
static_assert (arithmetic_type_flag<bl_u8>::get()  == malc_obj_u8, "");
static_assert (arithmetic_type_flag<bl_u16>::get() == malc_obj_u16, "");
static_assert (arithmetic_type_flag<bl_u32>::get() == malc_obj_u32, "");
static_assert (arithmetic_type_flag<bl_u64>::get() == malc_obj_u64, "");
static_assert (arithmetic_type_flag<bl_i8>::get()  == malc_obj_i8, "");
static_assert (arithmetic_type_flag<bl_i16>::get() == malc_obj_i16, "");
static_assert (arithmetic_type_flag<bl_i32>::get() == malc_obj_i32, "");
static_assert (arithmetic_type_flag<bl_i64>::get() == malc_obj_i64, "");
/*----------------------------------------------------------------------------*/
template <
  template <class, class...> class smartptr,
  template <class, class...> class container,
  class                            T,
  class...                         vectypes,
  class...                         smartptrtypes
  >
struct sertype<
  smartptr<container<T, vectypes...>, smartptrtypes...> ,
  typename std::enable_if<
    is_valid_builtin<T>::value &&
    is_valid_smartptr<smartptr>::value &&
    is_valid_container<container>::value
    >::type
  > :
  public smartptr_type_w_flag<
    container<T, vectypes...>,
    smartptr,
    arithmetic_type_flag<T>::get(),
    smartptrtypes...
    >
{};
/*----------------------------------------------------------------------------*/
}}} // namespace malcpp { namespace detail { namespace serialization {

#endif // __MALC_CPP_STD_TYPES_HPP__
