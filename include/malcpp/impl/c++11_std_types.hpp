#ifndef __MALC_CPP_STD_TYPES_HPP__
#define __MALC_CPP_STD_TYPES_HPP__

#include <sstream>
#include <memory>
#include <utility>
#include <type_traits>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <cassert>

#ifndef MALC_COMMON_NAMESPACED
#define MALC_COMMON_NAMESPACED 1
#endif

#include <bl/base/static_integer_math.h>

#include <malcpp/impl/c++11_basic_types.hpp>

// A header including C++ types. To avoid header bloat and to make a clear
// separation about what is C++ only.
#if 1
#warning "TODO: no pointer compression: fixed-user provided platform bytes"
#endif

#ifndef MALC_CPP_NULL_SMART_PTR_STR
  #define MALC_CPP_NULL_SMART_PTR_STR "nullptr"
#endif

namespace malcpp {

#define MALCPP_DECLARE_GET_DATA_FUNC(funcname) \
  int funcname( \
    malc_obj_ref*, \
    void const*, \
    malc_obj_push_context const*, \
    bl_alloc_tbl const* \
    )

extern MALC_EXPORT MALCPP_DECLARE_GET_DATA_FUNC (string_smartptr_get_data);
extern MALC_EXPORT MALCPP_DECLARE_GET_DATA_FUNC (vector_smartptr_get_data);
extern MALC_EXPORT MALCPP_DECLARE_GET_DATA_FUNC (ostringstream_get_data);

//------------------------------------------------------------------------------
namespace detail { namespace serialization {
//------------------------------------------------------------------------------
// OBJECT TYPES: interface types
//------------------------------------------------------------------------------
template <class T, bool is_rvalue>
struct value_pass {
public:
  value_pass() = delete;
  value_pass (value_pass&) = delete;
  value_pass (const value_pass&) = delete;

  value_pass (T&& pv) :v (std::move (pv)) {}
  value_pass (value_pass&& rv) :v (std::move (rv.v)) {}

  inline T&& move()
  {
    return std::move (v);
  }
private:
  T v;
};

template <class T>
struct value_pass<T, false> {
public:
  value_pass() = delete;
  value_pass (value_pass&) = delete;
  value_pass (const value_pass&) = delete;

  value_pass (T const& pv) :v (std::cref (pv)) {}
  value_pass (value_pass&& rv) :v (std::cref (rv.v)) {}

  inline T const& move()
  {
    return v.get();
  }
private:
  std::reference_wrapper<const T> v;
};
//------------------------------------------------------------------------------
}} //namespace detail { namespace serialization {
//------------------------------------------------------------------------------
template <class T, bool is_rvalue>
struct raw_object {
  using tref = typename std::conditional<is_rvalue, T&&, T const&>::type;

  raw_object (tref v, void const* t) : obj (std::forward<tref> (v))
  {
    table = t;
  }
  raw_object (raw_object&& rv) : raw_object (rv.obj.move(), rv.table)
  {}

  void const* table;
  detail::serialization::value_pass<T, is_rvalue> obj;
};
//------------------------------------------------------------------------------
template <class T, bool is_rvalue>
struct raw_object_w_context : public raw_object<T, is_rvalue> {
  using tref = typename raw_object<T, is_rvalue>::tref;

  raw_object_w_context (tref v, malc_obj_table const* t, void* c) :
    raw_object<T, is_rvalue> (std::forward<tref> (v), t)
  {
    context = c;
  }
  raw_object_w_context (raw_object_w_context&& rv) :
    raw_object_w_context (std::move (rv.obj), rv.table, rv.context)
  {}

  void* context;
};
//------------------------------------------------------------------------------
template <class T, bool is_rvalue>
struct raw_object_w_flag : public raw_object<T, is_rvalue> {
  using tref = typename raw_object<T, is_rvalue>::tref;

  raw_object_w_flag (tref v, malc_obj_table const* t, uint8_t f) :
    raw_object<T, is_rvalue> (std::forward<tref> (v), t)
  {
    flag = f;
  }
  raw_object_w_flag (raw_object_w_flag&& rv) :
    raw_object_w_flag (std::move (rv.obj), rv.table, rv.flag)
  {}

  uint8_t flag;
};
//------------------------------------------------------------------------------
namespace detail { namespace serialization {
//------------------------------------------------------------------------------
// OBJECT TYPES: serialization types
//------------------------------------------------------------------------------
template <class T>
struct wire_raw_object {
  static_assert(
    sizeof (T) <= MALC_OBJ_MAX_SIZE,
    "malc can only serialize objects from 0 to \"MALC_OBJ_MAX_SIZE\" bytes."
    );
  static_assert(
    alignof (T) <= MALC_OBJ_MAX_ALIGN,
    "malc can only serialize objects aligned up to \"MALC_OBJ_MAX_ALIGN\" bytes."
    );
#if MALC_PTR_COMPRESSION == 0
  void const* table;
#else
  malc_compressed_ptr table;
#endif
  typename std::aligned_storage<sizeof (T), alignof (T)>::type storage;
};

template <class T>
struct wire_raw_object_w_context : public wire_raw_object<T> {
#if MALC_PTR_COMPRESSION == 0
  void* context;
#else
  malc_compressed_ptr context;
#endif
};

template <class T>
struct wire_raw_object_w_flag : public wire_raw_object<T> {
  uint8_t flag;
};

static constexpr uint16_t cpp_std_types_compressed_count (uint8_t type_id)
{
#if MALC_PTR_COMPRESSION == 0
  return 0;
#else
  return
    ((uint16_t) (type_id == malc_type_obj)) +
    ((uint16_t) (type_id == malc_type_obj_flag)) +
    (((uint16_t) (type_id == malc_type_obj_ctx)) * 2);
#endif
}
//------------------------------------------------------------------------------
/* OBJECT TYPES: type transformations. It is possible to log these
(wire_raw_object_*) types directly, but it requires the user to provide the
"malc_obj_get_data_fn" and "malc_obj_destroy_fn" functions manually. */
//------------------------------------------------------------------------------
template <class T, template <class, bool> class external>
class obj_types_deduce_id_and_types;
//------------------------------------------------------------------------------
template <class T>
struct obj_types_deduce_id_and_types<T, raw_object> :
  public attach_type_id<malc_type_obj>
{
  template <bool value>
  using external_type = raw_object<remove_cvref_t<T>, value>;
  using serializable_type = wire_raw_object<remove_cvref_t<T> >;
};
//------------------------------------------------------------------------------
template <class T>
struct obj_types_deduce_id_and_types<T, raw_object_w_flag> :
  public attach_type_id<malc_type_obj_flag>
{
  template <bool value>
  using external_type = raw_object_w_flag<remove_cvref_t<T>, value>;
  using serializable_type = wire_raw_object_w_flag<remove_cvref_t<T> >;
};
//------------------------------------------------------------------------------
template <class T>
struct obj_types_deduce_id_and_types<T, raw_object_w_context> :
  public attach_type_id<malc_type_obj_ctx>
{
  template <bool value>
  using external_type = raw_object_w_context<remove_cvref_t<T>, value>;
  using serializable_type = wire_raw_object_w_context<remove_cvref_t<T> >;
};
//------------------------------------------------------------------------------
template <class T, template <class, bool> class external>
class obj_types_base : public obj_types_deduce_id_and_types<T, external> {
public:
  using base              = obj_types_deduce_id_and_types<T, external>;
  using serializable_type = typename base::serializable_type;
  template <bool rvalue>
  using external_type     = typename base::template external_type<rvalue>;

  static constexpr int compressed_count = serializable_type::compressed_count;
  //----------------------------------------------------------------------------
  static inline serializable_type to_serializable (external_type<true>&& v)
  {
    serializable_type s;
    fill_serializable (s, v);
    return s;
  }
  //----------------------------------------------------------------------------
  static inline serializable_type to_serializable (external_type<true>& v)
  {
    serializable_type s;
    fill_serializable (s, v);
    return s;
  }
  //----------------------------------------------------------------------------
  static inline serializable_type to_serializable (external_type<false>&& v)
  {
    serializable_type s;
    fill_serializable (s, v);
    return s;
  }
  //----------------------------------------------------------------------------
  static inline serializable_type to_serializable (external_type<false>& v)
  {
    serializable_type s;
    fill_serializable (s, v);
    return s;
  }
  //----------------------------------------------------------------------------
  static inline std::size_t wire_size (serializable_type const& v)
  {
    return size (v);
  };
  //----------------------------------------------------------------------------
  static inline void serialize (malc_serializer& s, serializable_type const& v)
  {
    do_serialize (s, v);
  }
  //----------------------------------------------------------------------------
private:
  //----------------------------------------------------------------------------
  template <bool is_rvalue>
  static inline void fill_serializable(
    wire_raw_object<T>& s, raw_object<T, is_rvalue>& v
    )
  {
    new (&s.storage) T (v.obj.move());
    s.table = get_logged_type<void*>::to_serializable ((void const*) v.table);
  }
  //----------------------------------------------------------------------------
  static inline std::size_t size (wire_raw_object<T> const& v)
  {
    return get_logged_type<void*>::wire_size (v.table) + sizeof (T);
  }
  //----------------------------------------------------------------------------
  static inline void do_serialize(
    malc_serializer& s, wire_raw_object<T> const& v
    )
  {
    get_logged_type<void*>::serialize (s, v.table);
    memcpy (s.field_mem, &v.storage, sizeof (T));
    s.field_mem += sizeof (T);
  }
  //----------------------------------------------------------------------------
  template <bool is_rvalue>
  static inline void fill_serializable(
    wire_raw_object_w_context<T>& s, raw_object_w_context<T, is_rvalue>& v
    )
  {
    fill_serializable(
      static_cast<wire_raw_object<T>&> (s),
      static_cast<raw_object<T, is_rvalue>&> (v)
      );
    s.table = get_logged_type<void*>::to_serializable (v.context);
  }
  //----------------------------------------------------------------------------
  static inline std::size_t size (wire_raw_object_w_context<T> const& v)
  {
    return size (static_cast<wire_raw_object<T> const&> (v)) +
      get_logged_type<void*>::wire_size (v.context);
  }
  //----------------------------------------------------------------------------
  static inline void do_serialize(
    malc_serializer& s, wire_raw_object_w_context<T> const& v
    )
  {
    get_logged_type<void*>::serialize (s, v.context);
    do_serialize (s, static_cast<wire_raw_object<T> const&> (v));
  }
  //----------------------------------------------------------------------------
  template <bool is_rvalue>
  static inline void fill_serializable(
    wire_raw_object_w_flag<T>& s, raw_object_w_flag<T, is_rvalue>& v
    )
  {
    fill_serializable(
      static_cast<wire_raw_object<T>&> (s),
      static_cast<raw_object<T, is_rvalue>&> (v)
      );
    s.flag = get_logged_type<uint8_t>::to_serializable (v.flag);
  }
  //----------------------------------------------------------------------------
  static inline std::size_t size (wire_raw_object_w_flag<T> const& v)
  {
    return size (static_cast<wire_raw_object<T> const&> (v)) +
      get_logged_type<uint8_t>::wire_size (v.flag);
  }
  //----------------------------------------------------------------------------
  static inline void do_serialize(
    malc_serializer& s, wire_raw_object_w_flag<T> const& v
    )
  {
    get_logged_type<uint8_t>::serialize (s, v.flag);
    do_serialize (s, static_cast<wire_raw_object<T> const&> (v));
  }
  //----------------------------------------------------------------------------
};
//------------------------------------------------------------------------------
template <class T, bool is_rvalue>
struct logged_type<raw_object<T, is_rvalue> > :
  public obj_types_base<T, raw_object>,
  public default_format_validation
{};
//------------------------------------------------------------------------------
template <class T, bool is_rvalue>
struct logged_type<raw_object_w_context<T, is_rvalue> > :
  public obj_types_base<T, raw_object_w_context>,
  public default_format_validation
{};
//------------------------------------------------------------------------------
template <class T, bool is_rvalue>
struct logged_type<raw_object_w_flag<T, is_rvalue> > :
  public obj_types_base<T, raw_object_w_flag>,
  public default_format_validation
{};
//------------------------------------------------------------------------------
// OBJECT TYPES: validation helpers
//------------------------------------------------------------------------------
template <template <class, class...> class smartptr>
struct is_valid_smartptr : public std::integral_constant<
  bool,
  std::is_same<smartptr<char>, std::shared_ptr<char> >::value ||
  std::is_same<smartptr<char>, std::weak_ptr<char> >::value ||
  std::is_same<smartptr<char>, std::unique_ptr<char> >::value
  >
{};

template <template <class, class...> class container>
struct is_valid_container : public std::integral_constant<
  bool,
  std::is_same<container<char>, std::vector<char> >::value
  >
{};
//------------------------------------------------------------------------------
// OBJECT TYPES: smartpr get_data functions
//------------------------------------------------------------------------------
template <class T, class enable = void>
struct smartpr_get_data_function;

template <template <class, class...> class smartptr, class... types>
struct smartpr_get_data_function<
    smartptr<std::string, types...>,
    typename std::enable_if<is_valid_smartptr<smartptr>::value>::type
  > {
  static constexpr malc_obj_get_data_fn value = string_smartptr_get_data;
};

template <
  template <class, class...> class smartptr, class T, class D, class... types
  >
struct smartpr_get_data_function<
    smartptr<std::vector<T, D>, types...>,
    typename std::enable_if<is_valid_smartptr<smartptr>::value>::type
  > {
  static constexpr malc_obj_get_data_fn value = vector_smartptr_get_data;
};

//------------------------------------------------------------------------------
template <class T>
struct dereference_smarptr;

template <class T>
struct dereference_smarptr<std::shared_ptr<T> > {
  static void* run (void* ptr)
  {
    return (void*) static_cast<std::shared_ptr<T>*> (ptr)->get();
  }
};

template <class T>
struct dereference_smarptr<std::weak_ptr<T> > {
  static void* run (void* ptr)
  {
    if (auto p = static_cast<std::weak_ptr<T>*> (ptr)->lock()) {
      return p.get();
    }
    return nullptr;
  }
};

template <class T, class D>
struct dereference_smarptr<std::unique_ptr<T, D> > {
  static void* run (void* ptr)
  {
    return static_cast<std::unique_ptr<T, D>*> (ptr)->get();
  }
};
//------------------------------------------------------------------------------
struct smartptr_table  {
  malc_obj_table table;
  void* (*dereference) (void* ptr);
};
//------------------------------------------------------------------------------
// OBJECT TYPES: Transformations for smart pointers.
//------------------------------------------------------------------------------
template<class ptrtype>
struct smartptr_obj : private obj_types_base<ptrtype, raw_object>
{
  //----------------------------------------------------------------------------
  using base = obj_types_base<ptrtype, raw_object>;
  //----------------------------------------------------------------------------
  using base::type_id;
  using typename base::serializable_type;
  using base::external_type;
  using base::serialize;
  using base::wire_size;

  using interface_type = ptrtype;
  //----------------------------------------------------------------------------
  static inline serializable_type to_serializable (interface_type const& v)
  {
    return base::to_serializable(
      raw_object<interface_type, false> (v, &table.table)
      );
  }
  //----------------------------------------------------------------------------
  static inline serializable_type to_serializable (interface_type&& v)
  {
    return base::to_serializable(
      raw_object<ptrtype, true> (std::forward<interface_type> (v), &table.table)
      );
  }
  //----------------------------------------------------------------------------
  static const smartptr_table table;
  //----------------------------------------------------------------------------
private:
  //----------------------------------------------------------------------------
  static void destroy (malc_obj_ref* obj, void const*)
  {
    static_cast<ptrtype*> (obj->obj)->~ptrtype();
  }
};
//------------------------------------------------------------------------------
template <class ptrtype>
const smartptr_table smartptr_obj<ptrtype>::table{
  .table = {
    smartpr_get_data_function<ptrtype>::value, destroy, sizeof (ptrtype)
  },
  .dereference = dereference_smarptr<ptrtype>::run,
};
//------------------------------------------------------------------------------
template<class ptrtype, uint8_t flag_default>
struct smartptr_obj_w_flag : private obj_types_base<ptrtype, raw_object_w_flag>
{
  //----------------------------------------------------------------------------
  using base = obj_types_base<ptrtype, raw_object_w_flag>;
  //----------------------------------------------------------------------------
  using base::type_id;
  using typename base::serializable_type;
  using base::external_type;
  using base::serialize;
  using base::wire_size;

  using interface_type = ptrtype;
  //----------------------------------------------------------------------------
  static inline serializable_type
    to_serializable (interface_type const& v, uint8_t flag = flag_default)
  {
    return base::to_serializable(
      raw_object_w_flag<interface_type, false> (v, &table->table, flag)
      );
  }
  //----------------------------------------------------------------------------
  static inline serializable_type
    to_serializable (interface_type&& v, uint8_t flag = flag_default)
  {
    return base::to_serializable(
      raw_object_w_flag<ptrtype, true>(
        std::forward<interface_type> (v), &table->table, flag
        )
      );
  }
  //----------------------------------------------------------------------------
private:
  static smartptr_table const * const table;
};
//------------------------------------------------------------------------------
template <class ptrtype, unsigned char flag>
smartptr_table const* const smartptr_obj_w_flag<ptrtype, flag>::table =
  &smartptr_obj<ptrtype>::table;
//------------------------------------------------------------------------------
// OBJECT TYPES: type instantiations of smart pointer types
//------------------------------------------------------------------------------
// "logged_type" for smart pointers of std::string.
template<template <class, class...> class smartptr, class... types>
struct logged_type<
  smartptr<std::string, types...>,
  typename std::enable_if<is_valid_smartptr<smartptr>::value>::type
  > :
  public smartptr_obj<smartptr<std::string, types...> >,
  public default_format_validation
{};
//------------------------------------------------------------------------------
template <class T>
struct arithmetic_type_flag {
  static constexpr uint8_t get()
  {
    return (bl_static_log2_ceil_u8 (sizeof (T))) |
      (std::is_signed<T>::value ? (1 << 2) : 0);
  }
};

template <>
struct arithmetic_type_flag<float> {
  static constexpr uint8_t get() { return malc_obj_float; }
};

template <>
struct arithmetic_type_flag<double> {
  static constexpr uint8_t get() { return malc_obj_double; }
};
//------------------------------------------------------------------------------
static_assert (arithmetic_type_flag<uint8_t>::get()  == malc_obj_u8, "");
static_assert (arithmetic_type_flag<uint16_t>::get() == malc_obj_u16, "");
static_assert (arithmetic_type_flag<uint32_t>::get() == malc_obj_u32, "");
static_assert (arithmetic_type_flag<uint64_t>::get() == malc_obj_u64, "");
static_assert (arithmetic_type_flag<int8_t>::get()   == malc_obj_i8, "");
static_assert (arithmetic_type_flag<int16_t>::get()  == malc_obj_i16, "");
static_assert (arithmetic_type_flag<int32_t>::get()  == malc_obj_i32, "");
static_assert (arithmetic_type_flag<int64_t>::get()  == malc_obj_i64, "");
//------------------------------------------------------------------------------
// "logged_type" for smart pointers containing std::vectors of arithmetic types
template <
  template <class, class...> class smartptr,
  template <class, class...> class container,
  class                            T,
  class...                         vectypes,
  class...                         smartptrtypes
  >
struct logged_type<
  smartptr<container<T, vectypes...>, smartptrtypes...> ,
  typename std::enable_if<
    is_valid_builtin<T>::value &&
    is_valid_smartptr<smartptr>::value &&
    is_valid_container<container>::value
    >::type
  > :
  public smartptr_obj_w_flag<
    smartptr<container<T, vectypes...>, smartptrtypes...>,
    arithmetic_type_flag<T>::get()
    >,
  public std::conditional<
    std::is_floating_point<T>::value,
    floating_point_format_validation,
    integral_format_validation
    >::type
{};
//------------------------------------------------------------------------------
// OBJECT TYPES: type instantiations for streamable types. Streamable types
// don't have its own logged_type, as they are logged by using the "ostr"
// function.
//------------------------------------------------------------------------------
template <class T, bool is_rvalue>
struct ostreamable {
  using tref = typename std::conditional<is_rvalue, T&&, T const&>::type;

  ostreamable (tref v) : obj (std::forward<tref> (v)) {};

  value_pass<T, is_rvalue> obj;
};
//------------------------------------------------------------------------------
template <class T, class enable = void>
struct ostreamable_printer {
  static bool run (void* ptr, std::ostringstream& o)
  {
    o << *static_cast<T*> (ptr);
    return true;
  }
};

template <template <class, class...> class smartptr, class T, class... types>
struct ostreamable_printer<
  smartptr<T, types...>,
  typename std::enable_if<is_valid_smartptr<smartptr>::value>::type
  >
{
  static bool run (void* ptr, std::ostringstream& o)
  {
    if (void* p = dereference_smarptr<smartptr<T, types...> >::run (ptr))
    {
      return ostreamable_printer<T>::run (p, o);
    }
    return false;
  }
};
//------------------------------------------------------------------------------
struct ostreamable_table  {
  malc_obj_table table;
  bool (*print) (void* ptr, std::ostringstream& o);
};
//------------------------------------------------------------------------------
template <class T>
class ostreamable_table_get {
public:
  static const ostreamable_table value;
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
};
template <class T>
const ostreamable_table ostreamable_table_get<T>::value{
  .table = {
    ostringstream_get_data, destroy, sizeof (T)
  },
  .print = ostreamable_printer<T>::run,
};
//------------------------------------------------------------------------------
}}} // namespace malcpp { namespace detail { namespace serialization {
//------------------------------------------------------------------------------

#endif // __MALC_CPP_STD_TYPES_HPP__
