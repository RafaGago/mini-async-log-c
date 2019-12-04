#ifndef __MALC_CPP_HPP__
#define __MALC_CPP_HPP__

#include <sstream>
#include <memory>
#include <cstddef>
#include <cassert>
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
#if 0
#warning "TODO: mutex wrapping or example about how to do it"
#warning "TODO: logging of typed arrays/vectors by value (maybe)"
#warning "TODO: logging of std::string"
#warning "TODO: ostream smart pointers"
#warning "TODO: Move the serialization and definitions to the C header"
#warning "TODO: allow obj types from C"
#warning "TODO: examples obj types from C"
#warning "TODO: examples obj types from C++"
#warning "TODO: wrapper for writing get_data_fn"
#warning "TODO: move "get_data" implementations to separate translation units"
#endif
#ifndef MALC_CPP_NULL_WEAK_PTR_STR
  #define MALC_CPP_NULL_WEAK_PTR_STR "null_weak_ptr"
#endif

namespace malcpp {

#define MALCPP_GET_DATA_FUNC_SIGNATURE(funcname) \
  void funcname( \
    malc_obj_ref*, \
    malc_obj_log_data*, \
    void**, \
    void const*, \
    char const*, \
    bl_alloc_tbl const* \
    )

extern MALC_EXPORT MALCPP_GET_DATA_FUNC_SIGNATURE (string_shared_ptr_get_data);
extern MALC_EXPORT MALCPP_GET_DATA_FUNC_SIGNATURE (vector_shared_ptr_get_data);
extern MALC_EXPORT MALCPP_GET_DATA_FUNC_SIGNATURE (string_weak_ptr_get_data);
extern MALC_EXPORT MALCPP_GET_DATA_FUNC_SIGNATURE (vector_weak_ptr_get_data);
extern MALC_EXPORT MALCPP_GET_DATA_FUNC_SIGNATURE (string_unique_ptr_get_data);
extern MALC_EXPORT MALCPP_GET_DATA_FUNC_SIGNATURE (vector_unique_ptr_get_data);
extern MALC_EXPORT MALCPP_GET_DATA_FUNC_SIGNATURE (ostringstream_get_data);

namespace detail { namespace serialization {

template <class T>
struct smartpr_get_data_function;

template <>
struct smartpr_get_data_function<std::shared_ptr<std::string> > {
  static constexpr malc_obj_get_data_fn value = string_shared_ptr_get_data;
};

template <class T, class... types>
struct smartpr_get_data_function<std::shared_ptr<std::vector<T, types...> > > {
  static constexpr malc_obj_get_data_fn value = vector_shared_ptr_get_data;
};

template <>
struct smartpr_get_data_function<std::weak_ptr<std::string> > {
  static constexpr malc_obj_get_data_fn value = string_weak_ptr_get_data;
};

template <class T, class... types>
struct smartpr_get_data_function<std::weak_ptr<std::vector<T, types...> > > {
  static constexpr malc_obj_get_data_fn value = vector_weak_ptr_get_data;
};

template <class ptrdeleter>
struct smartpr_get_data_function<std::unique_ptr<std::string, ptrdeleter> > {
  static constexpr malc_obj_get_data_fn value = string_unique_ptr_get_data;
};

template <class T, class ptrdeleter, class... types>
struct smartpr_get_data_function<
  std::unique_ptr<std::vector<T, types...>, ptrdeleter>
  > {
  static constexpr malc_obj_get_data_fn value = vector_unique_ptr_get_data;
};
/*----------------------------------------------------------------------------*/
/* interface types */
/*----------------------------------------------------------------------------*/
template <class T>
struct interface_obj {
  malc_obj_table const* table;
  T                     obj;
};

template <class T>
struct interface_obj_w_context : public interface_obj<T> {
  void* context;
};

template <class T>
struct interface_obj_w_flag : public interface_obj<T> {
  bl_u8 flag;
};
/*----------------------------------------------------------------------------*/
/* serialization types */
/*----------------------------------------------------------------------------*/
template <class T>
struct serializable_obj {
  static_assert(
    sizeof (T) <= 255,
    "malc can only serialize small objects from 0 to 255 bytes."
    );
  static_assert(
    alignof (T) <= MALC_OBJ_MAX_ALIGN,
    "malc can only serialize objects aligned up to \"std::max_align_t\"."
    );
#if MALC_PTR_COMPRESSION == 0
  malc_obj_table const* table;
#else
  malc_compressed_ptr table;
#endif
  typename std::aligned_storage<sizeof (T), alignof (T)>::type storage;
};

template <class T>
struct serializable_obj_w_context : public serializable_obj<T> {
#if MALC_PTR_COMPRESSION == 0
  void* context;
#else
  malc_compressed_ptr context;
#endif
};

template <class T>
struct serializable_obj_w_flag : public serializable_obj<T> {
  bl_u8 flag;
};
/*----------------------------------------------------------------------------*/
/* type transformations. It is possible to log these (serializable_obj_*)
types directly, but it requires the user to provide the "malc_obj_get_data_fn"
and "malc_obj_destroy_fn" functions manually. */
/*----------------------------------------------------------------------------*/
struct to_serialization_type_helper {
public:
  /* the interface type rvalues should contain the type to move on obj, lvalues
  a pointer, that's why this template has two parameters. See get_ref. */
  //----------------------------------------------------------------------------
  template <class T, class U>
  static inline void run (serializable_obj<T>& s, interface_obj<U>& v)
  {
    new (&s.storage) T (get_ref (v.obj));
#if MALC_PTR_COMPRESSION == 0
    s.table = v.table;
#else
    s.table = malc_get_compressed_ptr (v.table);
#endif
  }
  //----------------------------------------------------------------------------
  template <class T, class U>
  static inline void run(
    serializable_obj_w_context<T>& s, interface_obj_w_context<U>& v
    )
  {
    run(
      static_cast<serializable_obj<T>&> (s), static_cast<interface_obj<U>&> (v)
      );
#if MALC_PTR_COMPRESSION == 0
    s.context = v.context;
#else
    s.context = malc_get_compressed_ptr (v.context);
#endif
  }
  //----------------------------------------------------------------------------
  template <class T, class U>
  static inline void run(
    serializable_obj_w_flag<T>& s, interface_obj_w_flag<U>& v
    )
  {
    run(
      static_cast<serializable_obj<T>&> (s), static_cast<interface_obj<U>&> (v)
      );
    s.flag = v.flag;
  }
  //----------------------------------------------------------------------------
private:
  //----------------------------------------------------------------------------
  template <class T>
  static inline T& get_ref (T* ref)
  {
    return *ref;
  }
  //----------------------------------------------------------------------------
  template <class T>
  static inline T&& get_ref (T& ref)
  {
    return std::move (ref);
  }
  //----------------------------------------------------------------------------
};
/*----------------------------------------------------------------------------*/
template <
  class T,
  char malc_id,
  template <class> class interface_type,
  template <class> class serializable_type
  >
struct sertype_base {
public:
  /* the interface type rvalues should contain the type to move on obj,
  lvalues a pointer, but we will always have a serializable refering to the
  decayed type. */
  using serializable = serializable_type<typename std::remove_pointer<T>::type>;

  static constexpr char id = malc_id;

  static inline serializable to_serialization_type (interface_type<T>&& v)
  {
    serializable s;
    to_serialization_type_helper::run (s, v);
    return s;
  }

  static inline serializable to_serialization_type (interface_type<T>& v)
  {
    serializable s;
    to_serialization_type_helper::run (s, v);
    return s;
  }
};
/*----------------------------------------------------------------------------*/
template <class T>
struct sertype<interface_obj<T> > :
  public sertype_base<T, malc_type_obj, interface_obj, serializable_obj>
{};
/*----------------------------------------------------------------------------*/
template <class T>
struct sertype<interface_obj_w_context<T> > :
  public sertype_base<
    T, malc_type_obj_ctx, interface_obj_w_context, serializable_obj_w_context
    >
{};
/*----------------------------------------------------------------------------*/
template <class T>
struct sertype<interface_obj_w_flag<T> > :
  public sertype_base<
    T, malc_type_obj_flag, interface_obj_w_flag, serializable_obj_w_flag
    >
{};
/*----------------------------------------------------------------------------*/
//providing "type" for serializable C++ types. (See comment above).
/*----------------------------------------------------------------------------*/
template <class T>
struct sertype<serializable_obj<T> >{
  using serializable = serializable_obj<T>;

  static inline bl_uword size (serializable_obj<T> const& v)
  {
    assert (sizeof (T) == v.table->obj_sizeof);
    return
#if MALC_PTR_COMPRESSION == 0
      bl_sizeof_member (serializable, table)
#else
      malc_compressed_get_size (v.table.format_nibble)
#endif
      + sizeof (T);
  }
};
/*----------------------------------------------------------------------------*/
template <class T>
struct sertype<serializable_obj_w_context<T> > {
  using serializable = serializable_obj_w_context<T>;

  static inline bl_uword size (serializable const& v)
  {
    return sertype<serializable_obj<T> >::size (v)
#if MALC_PTR_COMPRESSION == 0
      + bl_sizeof_member (serializable, context);
#else
      + malc_compressed_get_size (v.context.format_nibble);
#endif
  }
};
/*----------------------------------------------------------------------------*/
template <class T>
struct sertype<serializable_obj_w_flag<T> > {
  using serializable = serializable_obj_w_flag<T>;

  static inline bl_uword size (serializable const& v)
  {
    return sertype<serializable_obj<T> >::size (v)
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
  malc_serializer* s, serializable_obj<T> const& v
  )
{
  assert (sizeof (T) == v.table->obj_sizeof);
  serialize_type (*s, v.table);
  memcpy (s->field_mem, &v.storage, sizeof (T));
  s->field_mem += sizeof (T);
}
/*----------------------------------------------------------------------------*/
template <class T>
static inline void malc_serialize(
  malc_serializer* s, serializable_obj_w_context<T> const& v
  )
{
  serialize_type (*s, v.context);
  malc_serialize (s, static_cast<serializable_obj<T> const&> (v));
}
/*----------------------------------------------------------------------------*/
template <class T>
static inline void malc_serialize(
  malc_serializer* s, serializable_obj_w_flag<T> const& v
  )
{
  serialize_type (*s, v.flag);
  malc_serialize (s, static_cast<serializable_obj<T> const&> (v));
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
template<class ptrtype>
struct smartptr_type  {
public:
  static constexpr char id = malc_type_obj;

  static inline serializable_obj<ptrtype> to_serialization_type (ptrtype& v)
  {
    using itype = interface_obj<ptrtype*>;
    itype i;
    set_base_data (i, v);
    return sertype<itype>::to_serialization_type (i);
  }

  static inline serializable_obj<ptrtype> to_serialization_type (ptrtype&& v)
  {
    using itype = interface_obj<ptrtype>;
    itype i;
    set_base_data (i, std::move (v));
    return sertype<itype>::to_serialization_type (std::move (i));
  }

  static inline void set_base_data (interface_obj<ptrtype*>& i, ptrtype& ptr)
  {
    i.obj = &ptr;
    i.table = &table;
  }

  static inline void set_base_data (interface_obj<ptrtype>& i, ptrtype&& ptr)
  {
    i.obj = std::move (ptr);
    i.table = &table;
  }
private:
  static void destroy (malc_obj_ref* obj, void const*)
  {
    static_cast<ptrtype*> (obj->obj)->~ptrtype();
  }
  static const malc_obj_table table;
};

template <class ptrtype>
const malc_obj_table smartptr_type<ptrtype>::table{
  .getdata    = smartpr_get_data_function<ptrtype>::value,
  .destroy    = destroy,
  .obj_sizeof = sizeof (ptrtype)
};
/*----------------------------------------------------------------------------*/
template< class ptrtype, unsigned char flag_default>
struct smartptr_type_w_flag {
  static constexpr char id = malc_type_obj_flag;

  static inline serializable_obj_w_flag<ptrtype>
    to_serialization_type (ptrtype& v, unsigned char flag = flag_default)
  {
    using itype = interface_obj_w_flag<ptrtype*>;
    itype i;
    smartptr_type<ptrtype>::set_base_data (i, v);
    i.flag = flag;
    return sertype<itype>::to_serialization_type (i);
  }

  static inline serializable_obj_w_flag<ptrtype>
    to_serialization_type (ptrtype&& v, unsigned char flag = flag_default)
  {
    using itype = interface_obj_w_flag<ptrtype>;
    itype i;
    smartptr_type<ptrtype>::set_base_data (i, std::move (v));
    i.flag = flag;
    return sertype<itype>::to_serialization_type (std::move (i));
  }
};
/*----------------------------------------------------------------------------*/
/* type instantiations of smart pointer types*/
/*----------------------------------------------------------------------------*/
// "sertype" for smart pointers of std::string.
template<template <class, class...> class smartptr, class... types>
struct sertype<
  smartptr<std::string, types...>,
  typename std::enable_if<is_valid_smartptr<smartptr>::value>::type
  > :
  public smartptr_type<smartptr<std::string, types...> >
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
// "sertype" for all classes of vectors, builtins and smart pointers.
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
    smartptr<container<T, vectypes...>, smartptrtypes...>,
    arithmetic_type_flag<T>::get()
    >
{};
/*----------------------------------------------------------------------------*/
/* type instantiations for streamable types */
/*----------------------------------------------------------------------------*/
template <class T>
struct ostreamable_lvalue {
  ostreamable_lvalue (T& v): ptr (&v) {}
  T* ptr;
};
/*----------------------------------------------------------------------------*/
template <class T>
struct ostreamable_rvalue {
  ostreamable_rvalue (T&& v): obj (std::move (v)) {}
  T obj;
};
/*----------------------------------------------------------------------------*/
/* This class is only done because if implementing it directly as "sertype" it
would require the "enable_if" twice, one for the class definition and another
one for the "static const malc_obj_table table" definition, which has to be
 done outside the class (C++11).*/

template <class T, template <class> class ostreamable>
struct sertype_ostreamable_impl {
  static constexpr char id = malc_type_obj_ctx;
  //----------------------------------------------------------------------------
  static inline serializable_obj_w_context<T>
    to_serialization_type (const ostreamable_lvalue<T>& v)
  {
    using itype = interface_obj_w_context<T*>;
    itype i;
    i.obj = v.ptr;
    i.table = &table;
    i.context = (void*) print;
    return sertype<itype>::to_serialization_type (i);
  }
  //----------------------------------------------------------------------------
  static inline serializable_obj_w_context<T>
    to_serialization_type (const ostreamable_rvalue<T>& v)
  {
    using itype = interface_obj_w_context<T>;
    itype i;
    i.obj = std::move (v.obj);
    i.table = &table;
    i.context = (void*) print;
    return sertype<itype>::to_serialization_type (std::move (i));
  }
  //----------------------------------------------------------------------------
private:
  //----------------------------------------------------------------------------
  static void destroy (malc_obj_ref* obj, void const*)
  {
    static_cast<T*> (obj->obj)->~T();
  }
  //----------------------------------------------------------------------------
  static void print (void* ptr, std::ostringstream& o)
  {
    o << *static_cast<T*> (ptr);
  }
  //----------------------------------------------------------------------------
  static const malc_obj_table table;
};

template <class T, template <class> class ostreamable>
const malc_obj_table sertype_ostreamable_impl<T, ostreamable>::table{
  .getdata    = ostringstream_get_data,
  .destroy    = destroy,
  .obj_sizeof = sizeof (T)
};
/*----------------------------------------------------------------------------*/
template <class T, template <class> class ostreamable>
struct sertype<
  ostreamable<T>,
  typename std::enable_if<
    std::is_same<ostreamable<T>, ostreamable_lvalue<T> >::value ||
    std::is_same<ostreamable<T>, ostreamable_rvalue<T> >::value
    >::type
   > : public sertype_ostreamable_impl<T, ostreamable>
{};
/*----------------------------------------------------------------------------*/
}} // namespace detail { namespace serialization {
/*----------------------------------------------------------------------------*/
template <class T>
detail::serialization::ostreamable_lvalue<T> ostr (T& v)
{
  return detail::serialization::ostreamable_lvalue<T> (v);
}
/*----------------------------------------------------------------------------*/
template <class T>
detail::serialization::ostreamable_rvalue<T> ostr (T&& v)
{
  return detail::serialization::ostreamable_rvalue<T> (std::move (v));
}
/*----------------------------------------------------------------------------*/
} // namespace malcpp {

#endif // __MALC_CPP_STD_TYPES_HPP__
