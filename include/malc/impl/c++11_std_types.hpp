#ifndef __MALC_CPP_HPP__
#define __MALC_CPP_HPP__

#include <sstream>
#include <memory>
#include <utility>
#include <type_traits>
#include <vector>
#include <cstddef>
#include <cassert>

#ifndef MALC_COMMON_NAMESPACED
#define MALC_COMMON_NAMESPACED 1
#endif

#include <bl/base/static_integer_math.h>

#include <malc/impl/serialization.hpp>

// A header including C++ types. To avoid header bloat and to make a clear
// separation about what is C++ only.
#if 1
#warning "TODO: mutex wrapping or example about how to do it"
#warning "TODO: logging of typed arrays/vectors by value (maybe)"
#warning "TODO: Move the object serialization and definitions to the C header. might require wrapping _Alignas"
#warning "TODO: allow string const references"
#warning "TODO: allow obj types from C"
#warning "TODO: examples obj types from C"
#warning "TODO: examples obj types from C++"
#endif
#ifndef MALC_CPP_NULL_SMART_PTR_STR
  #define MALC_CPP_NULL_SMART_PTR_STR "nullptr"
#endif

namespace malcpp {

#define MALCPP_DECLARE_GET_DATA_FUNC(funcname) \
  int funcname( \
    malc_obj_ref*, void const*, malc_obj_push_context const*, bl_alloc_tbl const* \
    )

extern MALC_EXPORT MALCPP_DECLARE_GET_DATA_FUNC (string_smartptr_get_data);
extern MALC_EXPORT MALCPP_DECLARE_GET_DATA_FUNC (vector_smartptr_get_data);
extern MALC_EXPORT MALCPP_DECLARE_GET_DATA_FUNC (ostringstream_get_data);

static inline detail::serialization::malc_strcp strcp (std::string& s);

namespace detail { namespace serialization {

/*----------------------------------------------------------------------------*/
/* STD STRING: special case to log "std::string" rvalues by copy, which is IMO
not very desirable on most cases for performance optimized code, but it sure
must have some legitimate uses. */
/*----------------------------------------------------------------------------*/
struct std_string_rvalue {
  std::string s;
};

struct std_string_rvalue_strcp {
  std_string_rvalue_strcp (std::string&& v)
  {
    s = std::move (v);
    sdata = strcp (s);
  }
  std_string_rvalue_strcp (std_string_rvalue_strcp&& v) :
    std_string_rvalue_strcp (std::move (v.s))
  {}

  std::string s;
  malc_strcp  sdata;
};

template<>
struct sertype<std_string_rvalue> {
public:
  static constexpr char id = malc_type_strcp;

  static inline std_string_rvalue_strcp
    to_serialization_type (std_string_rvalue& v)
  {
    return std_string_rvalue_strcp (std::move (v.s));
  }
  static inline std_string_rvalue_strcp
    to_serialization_type (std_string_rvalue&& v)
  {
    return std_string_rvalue_strcp (std::move (v.s));
  }
};

template<>
struct sertype<std_string_rvalue_strcp> {
  static inline bl_uword size (std_string_rvalue_strcp const& v)
  {
    return sertype<malc_strcp>::size (v.sdata);
  }
};

static inline void malc_serialize (malc_serializer* s, malc_strcp v);

static inline void malc_serialize(
  malc_serializer* s, std_string_rvalue_strcp const& v
  )
{
  malc_serialize (s, v.sdata);
}
/*----------------------------------------------------------------------------*/
}} //namespace detail { namespace serialization {
/*----------------------------------------------------------------------------*/
/* STD STRING: "std::string" rvalues by copy: user-function */
/*----------------------------------------------------------------------------*/
static inline detail::serialization::std_string_rvalue strcp (std::string&& s)
{
  return detail::serialization::std_string_rvalue{.s = std::move (s)};
}
/*----------------------------------------------------------------------------*/
namespace detail { namespace serialization {
/*----------------------------------------------------------------------------*/
/* OBJECT TYPES: interface types */
/*----------------------------------------------------------------------------*/
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
/*----------------------------------------------------------------------------*/
template <class T>
value_pass<T, false> valpass (T& v)
{
  return value_pass<T, false> (v);
}

template <class T>
value_pass<T, true> valpass (T&& v)
{
  return value_pass<T, true> (std::forward<T> (v));
}
/*----------------------------------------------------------------------------*/
template <class T, bool is_rvalue>
struct interface_obj {
  using tref = typename std::conditional<is_rvalue, T&&, T const&>::type;

  interface_obj (tref v, malc_obj_table const* t) : obj (std::forward<tref> (v))
  {
    table = t;
  }
    interface_obj (interface_obj&& rv) : obj (std::move (rv.obj))
  {
    table = rv.table;
  }
  malc_obj_table const*    table;
  value_pass<T, is_rvalue> obj;
};

template <class T, bool is_rvalue>
struct interface_obj_w_context : public interface_obj<T, is_rvalue> {
  using tref = typename interface_obj<T, is_rvalue>::tref;

  interface_obj_w_context (tref v, malc_obj_table const* t, void* c) :
    interface_obj<T, is_rvalue> (std::forward<tref> (v), t)
  {
    context = c;
  }
  interface_obj_w_context (interface_obj_w_context&& rv) :
    interface_obj<T, is_rvalue>(
      std::forward<interface_obj<T, is_rvalue> > (rv)
      )
  {
    context = rv.context;
  }
  void* context;
};

template <class T, bool is_rvalue>
struct interface_obj_w_flag : public interface_obj<T, is_rvalue> {
  using tref = typename interface_obj<T, is_rvalue>::tref;

  interface_obj_w_flag (tref v, malc_obj_table const* t, bl_u8 f) :
    interface_obj<T, is_rvalue> (std::forward<tref> (v), t)
  {
    flag = f;
  }
  interface_obj_w_flag (interface_obj_w_flag&& rv) :
    interface_obj<T, is_rvalue>(
      std::forward<interface_obj<T, is_rvalue> > (rv)
      )
  {
    flag = rv.flag;
  }
  bl_u8 flag;
};
/*----------------------------------------------------------------------------*/
/* OBJECT TYPES: serialization types */
/*----------------------------------------------------------------------------*/
template <class T>
struct serializable_obj {
  static_assert(
    sizeof (T) <= MALC_OBJ_MAX_SIZE,
    "malc can only serialize objects from 0 to \"MALC_OBJ_MAX_SIZE\" bytes."
    );
  static_assert(
    alignof (T) <= MALC_OBJ_MAX_ALIGN,
    "malc can only serialize objects aligned up to \"MALC_OBJ_MAX_ALIGN\" bytes."
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
/* OBJECT TYPES: type transformations. It is possible to log these
(serializable_obj_*) types directly, but it requires the user to provide the
"malc_obj_get_data_fn" and "malc_obj_destroy_fn" functions manually. */
/*----------------------------------------------------------------------------*/
struct to_serialization_type_helper {
public:
  //----------------------------------------------------------------------------
  template <class T, bool is_rvalue>
  static inline void run(
    serializable_obj<T>& s, interface_obj<T, is_rvalue>& v
    )
  {
    new (&s.storage) T (v.obj.move());
#if MALC_PTR_COMPRESSION == 0
    s.table = v.table;
#else
    s.table = malc_get_compressed_ptr (v.table);
#endif
  }
  //----------------------------------------------------------------------------
  template <class T, bool is_rvalue>
  static inline void run(
    serializable_obj_w_context<T>& s, interface_obj_w_context<T, is_rvalue>& v
    )
  {
    run(
      static_cast<serializable_obj<T>&> (s),
      static_cast<interface_obj<T, is_rvalue>&> (v)
      );
#if MALC_PTR_COMPRESSION == 0
    s.context = v.context;
#else
    s.context = malc_get_compressed_ptr (v.context);
#endif
  }
  //----------------------------------------------------------------------------
  template <class T, bool is_rvalue>
  static inline void run(
    serializable_obj_w_flag<T>& s, interface_obj_w_flag<T, is_rvalue>& v
    )
  {
    run(
      static_cast<serializable_obj<T>&> (s),
      static_cast<interface_obj<T, is_rvalue>&> (v)
      );
    s.flag = v.flag;
  }
  //----------------------------------------------------------------------------
};
/*----------------------------------------------------------------------------*/
/* OBJECT TYPES:
This class is the base implementation for the "sertype<interface_obj*>"
specialization family.

This class gets passed objects of the "interface_type" "type", which use to
contain all the required data to be able to serialize and print. Unfortunately
at this point we don't know if the contained object (to copy) was passed as
an rvalue or an lvalue.

As of now, to simplify things, if the value is a full object copy we assume it
was moved (rvalue) and we can move it again. If it doesn't we assumed the object
was passed by an lvalue. This is a HIDDEN CONVENTION to fix.*/
template <
  class T,
  char malc_id,
  template <class, bool> class interface_type,
  template <class> class serializable_type
  >
struct sertype_base {
public:
  using serializable   = serializable_type<T>;

  static constexpr char id = malc_id;

  template <bool is_rvalue>
  static inline serializable
    to_serialization_type (interface_type<T, is_rvalue>&& v)
  {
    serializable s;
    to_serialization_type_helper::run (s, v);
    return s;
  }

  template <bool is_rvalue>
  static inline serializable
    to_serialization_type (interface_type<T, is_rvalue>& v)
  {
    serializable s;
    to_serialization_type_helper::run (s, v);
    return s;
  }
};
/*----------------------------------------------------------------------------*/
template <class T, bool is_rvalue>
struct sertype<interface_obj<T, is_rvalue> > :
  public sertype_base<T, malc_type_obj, interface_obj, serializable_obj>
{};
/*----------------------------------------------------------------------------*/
template <class T, bool is_rvalue>
struct sertype<interface_obj_w_context<T, is_rvalue> > :
  public sertype_base<
    T, malc_type_obj_ctx, interface_obj_w_context, serializable_obj_w_context
    >
{};
/*----------------------------------------------------------------------------*/
template <class T, bool is_rvalue>
struct sertype<interface_obj_w_flag<T, is_rvalue> > :
  public sertype_base<
    T, malc_type_obj_flag, interface_obj_w_flag, serializable_obj_w_flag
    >
{};
/*----------------------------------------------------------------------------*/
}} // namespace detail { namespace serialization {
/*----------------------------------------------------------------------------*/
/* OBJECT TYPES: user-facing functions to serialize object types */
/*----------------------------------------------------------------------------*/
template <class T>
static detail::serialization::interface_obj<T, false>
  object (T const& v, malc_obj_table const* table)
{
  return detail::serialization::interface_obj<T, false> (v, table);
}

template <class T>
static detail::serialization::interface_obj<T, true>
  object (T&& v, malc_obj_table const* table)
{
  return detail::serialization::interface_obj<T, true>(
    std::forward<T> (v), table
    );
}

template <class T>
static detail::serialization::interface_obj_w_context<T, false>
  object (T const& v, malc_obj_table const* table, void* context)
{
  return detail::serialization::interface_obj_w_context<T, false>(
    v, table, context
    );
}

template <class T>
static detail::serialization::interface_obj_w_context<T, true>
  object (T&& v, malc_obj_table const* table, void* context)
{
  return detail::serialization::interface_obj_w_context<T, true>(
    std::forward<T> (v), table, context
    );
}

template <class T>
static detail::serialization::interface_obj_w_flag<T, false>
  object (T const& v, malc_obj_table const* table, bl_u8 flag)
{
  return detail::serialization::interface_obj_w_flag<T, false>(
    v, table, flag
    );
}

template <class T>
static detail::serialization::interface_obj_w_flag<T, true>
  object (T&& v, malc_obj_table const* table, bl_u8 flag)
{
  return detail::serialization::interface_obj_w_flag<T, true>(
    std::forward<T> (v), table, flag
    );
}
/*----------------------------------------------------------------------------*/
namespace detail { namespace serialization {
/*----------------------------------------------------------------------------*/
//OBJECT TYPES: providing some C++ types on the standard library.
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
/* //OBJECT TYPES: serializations */
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
/* OBJECT TYPES: validation helpers */
/*----------------------------------------------------------------------------*/
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

template <class T>
struct is_valid_builtin : public std::integral_constant<
  bool,
  std::is_arithmetic<T>::value && ! std::is_same<T, bool>::value
  >
{};
/*----------------------------------------------------------------------------*/
/* OBJECT TYPES: smartpr get_data functions */
/*----------------------------------------------------------------------------*/
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

/*----------------------------------------------------------------------------*/
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
/*----------------------------------------------------------------------------*/
struct smartptr_table  {
  malc_obj_table table;
  void* (*dereference) (void* ptr);
};
/*----------------------------------------------------------------------------*/
/* OBJECT TYPES: Transformations for smart pointers. These create a
serialization-friendly type directly from C++ types.
*/
/*----------------------------------------------------------------------------*/
template<class ptrtype>
struct smartptr_type  {
public:
  static constexpr char id = malc_type_obj;

  static inline serializable_obj<ptrtype>
    to_serialization_type (ptrtype const& v)
  {
    return to_serialization_type_base (object (v, &table.table));
  }

  static inline serializable_obj<ptrtype> to_serialization_type (ptrtype&& v)
  {
    return to_serialization_type_base(
      object (std::forward<ptrtype> (v), &table.table)
      );
  }

  static const smartptr_table table;

private:
  template <class T>
  static serializable_obj<ptrtype> to_serialization_type_base (T&& v)
  {
    return sertype<T>::to_serialization_type (std::forward<T> (v));
  }

  static void destroy (malc_obj_ref* obj, void const*)
  {
    static_cast<ptrtype*> (obj->obj)->~ptrtype();
  }
};

template <class ptrtype>
const smartptr_table smartptr_type<ptrtype>::table{
  .table = {
    smartpr_get_data_function<ptrtype>::value, destroy, sizeof (ptrtype)
  },
  .dereference = dereference_smarptr<ptrtype>::run,
};
/*----------------------------------------------------------------------------*/
template< class ptrtype, unsigned char flag_default>
struct smartptr_type_w_flag {
public:
  static constexpr char id = malc_type_obj_flag;

  static smartptr_table const* const table;

  static inline serializable_obj_w_flag<ptrtype>
    to_serialization_type (ptrtype const & v, unsigned char flag = flag_default)
  {
    return to_serialization_type_base (object (v, &table->table, flag));
  }

  static inline serializable_obj_w_flag<ptrtype>
    to_serialization_type (ptrtype&& v, unsigned char flag = flag_default)
  {
    return to_serialization_type_base(
      object (std::forward<ptrtype> (v), &table->table, flag)
      );
  }
private:
  template <class T>
  static serializable_obj_w_flag<ptrtype> to_serialization_type_base (T&& v)
  {
    return sertype<T>::to_serialization_type (std::forward<T> (v));
  }
};

template <class ptrtype, unsigned char flag>
smartptr_table const* const smartptr_type_w_flag<ptrtype, flag>::table =
  &smartptr_type<ptrtype>::table;
/*----------------------------------------------------------------------------*/
/* OBJECT TYPES: type instantiations of smart pointer types*/
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
/* OBJECT TYPES: type instantiations for streamable types */
/*----------------------------------------------------------------------------*/
template <class T, bool is_rvalue>
struct ostreamable {
  using tref = typename std::conditional<is_rvalue, T&&, T const&>::type;

  ostreamable (tref v) : obj (std::forward<tref> (v)) {};

  value_pass<T, is_rvalue> obj;
};
/*----------------------------------------------------------------------------*/
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
/*----------------------------------------------------------------------------*/
struct ostreamable_table  {
  malc_obj_table table;
  bool (*print) (void* ptr, std::ostringstream& o);
};
/*----------------------------------------------------------------------------*/
template <class T, bool is_rvalue>
struct ostreamable_impl {
public:
  static constexpr char id = malc_type_obj;
  //----------------------------------------------------------------------------
  static inline serializable_obj<T>
    to_serialization_type (ostreamable<T, is_rvalue>& v)
  {
    return to_serialization_type_base(
      detail::serialization::interface_obj<T, is_rvalue>(
        v.obj.move(), &table.table
        ));
  }
  //----------------------------------------------------------------------------
  static inline serializable_obj<T>
    to_serialization_type (ostreamable<T, is_rvalue>&& v)
  {
    return to_serialization_type_base(
      detail::serialization::interface_obj<T, is_rvalue>(
        v.obj.move(), &table.table
        ));
  }
  //----------------------------------------------------------------------------
private:
  template <class U>
  static serializable_obj<T> to_serialization_type_base (U&& v)
  {
    return sertype<U>::to_serialization_type (std::forward<U> (v));
  }
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
  static const ostreamable_table table;
};
template <class T, bool is_rvalue>
const ostreamable_table ostreamable_impl<T, is_rvalue>::table{
  .table = {
    ostringstream_get_data, destroy, sizeof (T)
  },
  .print = ostreamable_printer<remove_cvref_t<T> >::run,
};
/*----------------------------------------------------------------------------*/
template <class T, bool is_rvalue>
struct builtin_use_no_ostream {
  using builtin = remove_cvref_t<T>;
  static constexpr char id = sertype<builtin>::id;
  //----------------------------------------------------------------------------
  static inline builtin to_serialization_type (ostreamable<T, is_rvalue>& v)
  {
    return v.obj.move();
  }
  //----------------------------------------------------------------------------
  static inline builtin to_serialization_type (ostreamable<T, is_rvalue>&& v)
  {
    return v.obj.move();
  }
  //----------------------------------------------------------------------------
};
/*----------------------------------------------------------------------------*/
template <class T, bool is_rvalue>
struct sertype<ostreamable<T, is_rvalue> > :
  public std::conditional<
    !is_valid_builtin<remove_cvref_t<T> >::value,
    ostreamable_impl<T, is_rvalue>,
    builtin_use_no_ostream<T, is_rvalue>
    >::type
{};
/*----------------------------------------------------------------------------*/
}} // namespace detail { namespace serialization {
/*----------------------------------------------------------------------------*/
// OBJECT TYPES: user-facing functions to log stream objects.
/*----------------------------------------------------------------------------*/
template <class T>
detail::serialization::ostreamable<T, false> ostr (T& v)
{
  return detail::serialization::ostreamable<T, false> (v);
}
/*----------------------------------------------------------------------------*/
template <class T>
detail::serialization::ostreamable<T, true> ostr (T&& v)
{
  return detail::serialization::ostreamable<T, true> (std::forward<T> (v));
}
/*----------------------------------------------------------------------------*/

} // namespace malcpp {

#endif // __MALC_CPP_STD_TYPES_HPP__
