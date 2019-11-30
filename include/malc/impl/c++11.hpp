#ifndef __MALC_CPP11__
#define __MALC_CPP11__

#ifndef MALC_COMMON_NAMESPACED
  #error "Don't include this file directly"
#endif

#include <tuple>
#include <utility>

#include <bl/base/preprocessor_basic.h>
// #include <bl/base/integer.h>

//#include <malc/impl/metaprogramming_common.hpp>
#include <malc/impl/serialization.hpp>
#include <malc/impl/compile_time_validation.hpp>

#include <malc/impl/logging.h>

namespace malcpp { namespace detail {

/*----------------------------------------------------------------------------*/
template <int sev, class T>
struct info {};

template <int sev, template <class...> class T, class... Args>
struct info<sev, T<Args...> >
{
  static inline const char* generate()
  {
    static const char info[] = {
      (char) sev,
      (::malcpp::detail::serialization::type<remove_cvref_t<Args> >::id) ...,
      0
    };
    return info;
  }
};
/*----------------------------------------------------------------------------*/
template <int END, int N = 0>
class arg_ops {
public:
  //----------------------------------------------------------------------------
  template <class T>
  static inline bl_uword get_payload_size (const T& tup)
  {
    bl_uword v = serialization::type<
      remove_cvref_t<decltype (std::get<N>(tup))>
        >::size (std::get<N>(tup));
    return v + arg_ops<END, N + 1>::get_payload_size (tup);
  }
  //----------------------------------------------------------------------------
  template <class T>
  static inline void serialize (malc_serializer& ser, const T& tup)
  {
    serialization::malc_serialize (&ser, std::get<N>(tup));
    arg_ops<END, N + 1>::serialize (ser, tup);
  }
  //----------------------------------------------------------------------------
  template <class T, int... refs>
  static inline void try_deallocate_refvalues(
    const T& tup, const serialization::malc_refdtor* dtor, intlist<refs...>
    )
  {
    using type = decltype (std::get<N> (tup));
    arg_ops<END, N + 1>::try_deallocate_refvalues(
      tup,
      arg_ops<END, N>::get_refdtor_pointer (std::get<N> (tup)),
      typename std::conditional<
        serialization::is_malc_refvalue<type>::value,
        intlist<refs..., N>,
        intlist<refs...>
        >::type()
      );
  }
  //----------------------------------------------------------------------------
private:
  //----------------------------------------------------------------------------
  template <class T>
  static inline const serialization::malc_refdtor* get_refdtor_pointer (T&)
  {
    return nullptr;
  }
  //----------------------------------------------------------------------------
  static inline const serialization::malc_refdtor* get_refdtor_pointer(
    const serialization::malc_refdtor& rd
    )
  {
    return &rd;
  }
  //----------------------------------------------------------------------------
};

template <int END>
class arg_ops<END, END> {
public:
  //----------------------------------------------------------------------------
  template <class T>
  static inline bl_uword get_payload_size (const T& tup)
  {
    return 0;
  }
  //----------------------------------------------------------------------------
  template <class T>
  static inline void serialize (malc_serializer& ser, const T& tup)
  {}
  //----------------------------------------------------------------------------
  template <class T, int... refs>
  static inline void try_deallocate_refvalues(
    const T& tup, const serialization::malc_refdtor* dtor, intlist<refs...>
    )
  {
    static const int refscount = intlist_sizeof<intlist<refs...> >::value;
    if (refscount > 0 && dtor && dtor->func) {
      malc_ref r[] = { to_malc_ref (std::get<refs> (tup))... };
      dtor->func (dtor->context, r, refscount);
    }
  }
  //----------------------------------------------------------------------------
private:
  //----------------------------------------------------------------------------
  static inline malc_ref to_malc_ref (const serialization::malc_memref& v
    )
  {
    malc_ref r = { v.mem, v.size };
    return r;
  }
  //----------------------------------------------------------------------------
  static inline malc_ref to_malc_ref (const serialization::malc_strref& v)
  {
    malc_ref r = { v.str, v.len };
    return r;
  }
  //----------------------------------------------------------------------------
};
/*----------------------------------------------------------------------------*/
template <class T>
static inline void deallocate_refs_tuple (std::true_type has_refs, const T& tup)
{
   arg_ops<std::tuple_size<T>::value>::try_deallocate_refvalues(
     tup, nullptr, intlist<>()
     );
}
/*----------------------------------------------------------------------------*/
template <class T>
static inline void deallocate_refs_tuple(
  std::false_type has_refs, const T& tup
  )
{}
/*----------------------------------------------------------------------------*/
template <class... types>
static inline void deallocate_refs(
  std::true_type has_refs, const char*, types... args
  )
{
  deallocate_refs_tuple (has_refs, std::make_tuple (args...));
}
/*----------------------------------------------------------------------------*/
template <class... types>
static inline void deallocate_refs (std::false_type, types...)
{
  // just to avoid unneeded compiler iterations.
}
/*----------------------------------------------------------------------------*/
template <class has_refs, class malctype, class... types>
static inline bl_err log(
  has_refs,
  malc_const_entry const& en,
  malctype&               malc,
  const char*,
  types...                args
  )
{
  using argops = arg_ops<sizeof...(types)>;
  auto values = std::make_tuple(
    serialization::type<types>::transform (std::forward<types> (args))...
    );
  malc_serializer s;
  bl_err err = malc_log_entry_prepare(
    malc.handle(), &s, &en, argops::get_payload_size (values)
    );
  if (err.own) {
#if MALC_PTR_COMPRESSION == 0
    deallocate_refs_tuple (has_refs(), values);
#else
    deallocate_refs(
      has_refs(), (const char*) nullptr, std::forward (args)...);
#endif
    return err;
  }
  argops::serialize (s, values);
  malc_log_entry_commit (malc.handle(), &s);
  return err;
}
//------------------------------------------------------------------------------
}} // namespace malcpp { namespace detail {
/*----------------------------------------------------------------------------*/
/* Implementation of  MALC_LOG_IF_PRIVATE/MALC_LOG_PRIVATE */
/*----------------------------------------------------------------------------*/
/* Reminder: This can't be done without macros because:

-The arguments must be only evaluated (lazily) when logging, not when the entry
 is filtered out.
-On C++11 the literal can't be passed on template parameters. It decays early.
 */
#define MALC_LOG_IF_PRIVATE(cond, malcref, sev, ...) \
[&]() { \
  bl_err err = bl_mkok(); \
  using argtypelist = decltype (malcpp::detail::make_typelist( \
        bl_pp_vargs_ignore_first (__VA_ARGS__) /*1st arg = format str*/ \
        )); \
  /* Trigger reference values and string literal validation */ \
  static constexpr bool has_references = \
    ::malcpp::detail::fmt::static_arg_validation< \
      ::malcpp::detail::fmt::refvalues::validate<argtypelist>(), \
      ::malcpp::detail::fmt::format_string::validate<argtypelist>( \
          bl_pp_vargs_first (__VA_ARGS__) /*1st arg = format str*/\
          ) \
    >() == ::malcpp::detail::fmt::fmterr_success_with_refs; \
  if ((cond) && (sev) >= malc_get_min_severity (malcref.handle())) { \
    static const malc_const_entry msgdata =  {\
      bl_pp_vargs_first (__VA_ARGS__), /*1st arg = format str*/\
      ::malcpp::detail::info<sev, argtypelist>::generate(), \
      0 \
    }; \
    err = ::malcpp::detail::log( \
      std::integral_constant<bool, has_references>(), \
      msgdata, \
      malcref, \
      __VA_ARGS__ \
      ); \
  } \
  else if (has_references) { \
    ::malcpp::detail::deallocate_refs( \
      std::integral_constant<bool, has_references>(), __VA_ARGS__ \
      ); \
  } \
  return err; \
}()
/*----------------------------------------------------------------------------*/
#define MALC_LOG_PRIVATE(malcref, sev, ...) \
    MALC_LOG_IF_PRIVATE (1, (malcref), (sev), __VA_ARGS__)
/*----------------------------------------------------------------------------*/
#endif /* __MALC_CPP11__ */
