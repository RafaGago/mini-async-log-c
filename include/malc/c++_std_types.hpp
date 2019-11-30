#ifndef __MALC_CPP_STD_TYPES_HPP__
#define __MALC_CPP_STD_TYPES_HPP__

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
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context
  );
extern MALC_EXPORT void integral_vector_shared_ptr_get_data(
  malc_obj_ref* obj, malc_obj_log_data* out, void** iter_context
  );

namespace detail { namespace serialization {

/*----------------------------------------------------------------------------*/
/* object types (C++ only) */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* interface types */
/*----------------------------------------------------------------------------*/
template <class T>
struct malc_obj_shared_ptr {
  malc_obj_get_data_fn getdata;
  malc_obj_destroy_fn  destroy;
  std::shared_ptr<T>   ptr;
};

template <class T>
struct malc_obj_shared_ptr_w_context : public malc_obj_shared_ptr<T> {
  void* context;
};

template <class T>
struct malc_obj_shared_ptr_w_flag : public malc_obj_shared_ptr<T> {
  bl_u8 flag;
};
/*----------------------------------------------------------------------------*/
/* serialization (transformed) types */
/*----------------------------------------------------------------------------*/
template <class T>
struct malc_transformed_obj {
#if MALC_PTR_COMPRESSION == 0
  malc_obj_get_data_fn   getdata;
  malc_obj_destroy_fn    destroy;
#else
  malc_compressed_ptr    getdata;
  malc_compressed_ptr    destroy;
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
/* type transformations (base)*/
/*----------------------------------------------------------------------------*/
template <class T>
struct type<malc_obj_shared_ptr<T> > {

  static_assert(
    sizeof (std::shared_ptr<T>) <= 255,
    "malc can only serialize small objects from 0 to 255 bytes."
    );
  static_assert(
    alignof (std::shared_ptr<T>) <= MALC_OBJ_MAX_ALIGN,
    "malc can only serialize objects aligned up to \"std::max_align_t\"."
    );

  using transformed = malc_transformed_obj<std::shared_ptr<T> >;

  static constexpr char id = malc_type_obj;

  static inline transformed transform (malc_obj_shared_ptr<T>& v)
  {
    transformed r;
    new (&r.storage) std::shared_ptr<T> (std::move (v.ptr));
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
template <class T>
struct type<malc_obj_shared_ptr_w_context<T> > {

  using base = type<malc_obj_shared_ptr<T> >;
  using transformed = malc_transformed_obj_w_context<std::shared_ptr<T> >;

  static constexpr char id = malc_type_obj_ctx;

  static inline transformed transform (malc_obj_shared_ptr_w_context<T>& v)
  {
    transformed r;
    r.base = base::transform (static_cast<malc_obj_shared_ptr<T>&> (v));
#if MALC_PTR_COMPRESSION == 0
    r.context = v.context;
#else
    r.context = malc_get_compressed_ptr (v.context);
#endif
    return r;
  }
};
/*----------------------------------------------------------------------------*/
template <class T>
struct type<malc_obj_shared_ptr_w_flag<T> > :
  private type<malc_obj_shared_ptr<T> > {

  using base = type<malc_obj_shared_ptr<T> >;
  using transformed = malc_transformed_obj_w_flag<std::shared_ptr<T> >;

  static constexpr char id = malc_type_obj_flag;

  static inline transformed transform (malc_obj_shared_ptr_w_flag<T>& v)
  {
    transformed r;
    r.base = base::transform (static_cast<malc_obj_shared_ptr<T>&> (v));
    r.flag = v.flag;
    return r;
  }
};
/*----------------------------------------------------------------------------*/
/* type transformations (real types)*/
/*----------------------------------------------------------------------------*/
template<>
struct type<std::shared_ptr<std::string> > :
  private type<malc_obj_shared_ptr<std::string> > {

  using base = type<malc_obj_shared_ptr<std::string> >;

  using base::id;

  static void destroy (malc_obj_ref* obj)
  {
    using T = std::shared_ptr<std::string>;
    static_cast<T*> (obj->obj)->~T();
  }

  static inline base::transformed transform (std::shared_ptr<std::string>& v)
  {
    malc_obj_shared_ptr<std::string> b;
    b.ptr = v;
    b.getdata = &string_shared_ptr_get_data;
    b.destroy = destroy;
    return base::transform (b);
  }

  static inline base::transformed transform (std::shared_ptr<std::string>&& v)
  {
    malc_obj_shared_ptr<std::string> b;
    b.ptr = std::move (v);
    b.getdata = &string_shared_ptr_get_data;
    b.destroy = destroy;
    return base::transform (b);
  }
};
/*----------------------------------------------------------------------------*/
template <malc_obj_get_data_fn getdata, class colwrapper>
struct shared_ptr_collection_type :
  private type<malc_obj_shared_ptr_w_flag<typename colwrapper::collection> > {

  using coltype    = typename colwrapper::type;
  using collection = typename colwrapper::collection;

  using base = type<malc_obj_shared_ptr_w_flag<collection> >;
  using base::id;

  static void destroy (malc_obj_ref* obj)
  {
    /* unfortunately it can't be moved to the .cpp, as the collections have other
    template parameters than the data-type (e.g. allocators).*/
    using T = std::shared_ptr<collection>;
    static_cast<T*> (obj->obj)->~T();
  }

  static inline constexpr bl_u8 encode_type_flag()
  {
    return (bl_static_log2_ceil_u8 (sizeof (coltype))) |
      (std::is_signed<coltype>::value ? (1 << 2) : 0);
  }

  static inline typename base::transformed transform(
    std::shared_ptr<collection>& v
    )
  {
    malc_obj_shared_ptr_w_flag<collection> b;
    b.ptr     = v;
    b.getdata = getdata;
    b.destroy = &destroy;
    b.flag    = encode_type_flag();
    return base::transform (b);
  }

  static inline typename base::transformed transform(
    std::shared_ptr<collection>&& v
    )
  {
    malc_obj_shared_ptr_w_flag<collection> b;
    b.ptr     = std::move (v);
    b.getdata = getdata;
    b.destroy = &destroy;
    b.flag    = encode_type_flag();
    return base::transform (b);
  }
};
/*----------------------------------------------------------------------------*/
template <class T, class... Args>
struct vector_wrapper {
  using collection = std::vector<T, Args...>;
  using type       = T;
};
/*----------------------------------------------------------------------------*/
template <class T, class... Args>
struct type<
  std::shared_ptr<std::vector<T, Args...> > ,
  typename std::enable_if<
    std::is_integral<T>::value && !std::is_same<T, bool>::value
    >::type
  > :
  public shared_ptr_collection_type<
    integral_vector_shared_ptr_get_data, vector_wrapper<T, Args...>
    >
{};
/*----------------------------------------------------------------------------*/
/* transformed type sizes */
/*----------------------------------------------------------------------------*/
template <class T>
struct type<malc_transformed_obj<std::shared_ptr<T> > > {
  using transformed = malc_transformed_obj<std::shared_ptr<T> >;

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
      + sizeof (std::shared_ptr<T>);
  }
};
/*----------------------------------------------------------------------------*/
template <class T>
struct type<malc_transformed_obj_w_context<std::shared_ptr<T> > > {

  using base = type<malc_transformed_obj<std::shared_ptr<T> > >;
  using transformed = malc_transformed_obj_w_context<std::shared_ptr<T> >;

  static inline bl_uword size (const transformed& v)
  {
    return base::size (v.base)
#if MALC_PTR_COMPRESSION == 0
      + bl_sizeof_member (malc_obj_shared_ptr_w_context<T>, context);
#else
      + malc_compressed_get_size (v.context.format_nibble);
#endif
  }
};
/*----------------------------------------------------------------------------*/
template <class T>
struct type<malc_transformed_obj_w_flag<std::shared_ptr<T> > > {

  using base = type<malc_transformed_obj<std::shared_ptr<T> > >;
  using transformed = malc_transformed_obj_w_flag<std::shared_ptr<T> >;

  static inline bl_uword size (const transformed& v)
  {
    return base::size (v.base)
      + bl_sizeof_member (malc_obj_shared_ptr_w_flag<T>, flag);
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
