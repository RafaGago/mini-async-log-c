#ifndef __MALC_CPP11_BASIC_TYPES_HPP__
#define __MALC_CPP11_BASIC_TYPES_HPP__

#include <cstdint>
#include <utility>
#include <string>

#include <malcpp/impl/serialization.hpp>

namespace malcpp { namespace detail { namespace serialization {
//------------------------------------------------------------------------------
template <class T, char id>
struct reuse_all_possible_from_c_impl :
  public attach_type_id<id>,
  public serializable_conversion_and_types_from_c_impl<T>,
  public wire_size_from_c_impl<
    typename serializable_conversion_and_types_from_c_impl<T>::serializable_type
    >,
  public serialization_from_c_impl<
    typename serializable_conversion_and_types_from_c_impl<T>::serializable_type
    >
{};
//------------------------------------------------------------------------------
struct floating_point_format_validation;
//------------------------------------------------------------------------------
struct integral_format_validation {
public:
  //----------------------------------------------------------------------------
  static constexpr bool validate_format_modifiers (const literal& l)
  {
    return (l.size() == 0)
      ? true
      : validate_flags (l, 0, 0, l.size(), literal (" #+-0"));
  }
  //----------------------------------------------------------------------------
private:
  //----------------------------------------------------------------------------
  friend struct floating_point_format_validation;
  //----------------------------------------------------------------------------
  static constexpr bool validate_specifiers(
    const literal& l, int beg, int end, const literal& modf
    )
  {
    return
      (beg < end)
      ? (end - beg == 1)
        ? (modf.find (l[beg], 0, modf.size(), -1) != -1)
          //keep validating
          ? validate_specifiers (l, beg + 1, end, modf)
          //unknown modifier
          : false
        //only one specifier allowed
        : false
      : true;
  }
  //----------------------------------------------------------------------------
  static constexpr bool validate_precision(
    const literal& l,
    int stgbeg,
    int beg,
    int end,
    const literal& modf,
    int maxchars = 3 /* matches the implementation */
    )
  {
    return
      (beg < end)
      ? ((beg - stgbeg) > maxchars)
        ? false // too wide
        : (modf.find (l[beg], 0, modf.size(), -1) != -1)
          // current stage
          ? (l[beg] == '.' && beg != stgbeg)
            ? false /* dot not in beggining */
            : (l[beg] == 'w' && beg != stgbeg + 1)
              ? false /* 'w' not after '.' */
              : ((l[beg] >= '0' && l[beg] <= '9') &&
                  ((l[beg + 1] == 'w') || beg == stgbeg))
                ? false /* 0-9 after 'w' */
                : validate_precision(l, stgbeg, beg + 1, end, modf)
          // next stage
          : ((beg - stgbeg) != 1)
            ? validate_specifiers (l, beg, end, literal ("xXo"))
            : false
      : ((beg - stgbeg) > 1) && ((beg - stgbeg) <= maxchars);
  }
  //----------------------------------------------------------------------------
  static constexpr bool validate_width(
    const literal& l,
    int stgbeg,
    int beg,
    int end,
    const literal& modf,
    int maxchars = 2 /* matches the implementation */
    )
  {
    return
      (beg < end)
      ? ((beg - stgbeg) > maxchars)
        ? false // too wide
        : (modf.find (l[beg], 0, modf.size(), -1) != -1)
          ? validate_width (l, stgbeg, beg + 1, end, modf)
          : validate_precision (l, beg, beg, end, literal (".w0123456789"))
      : ((beg - stgbeg) <= maxchars) ;
  }
  //----------------------------------------------------------------------------
  static constexpr bool validate_flags(
    const literal& l, int stgbeg, int beg, int end, const literal& modf
    )
  {
    // this function validates the flags or jumps to the width modifiers
    return
      (beg < end)
      ? (modf.find (l[beg], 0, modf.size(), -1) != -1)
        //keep validating the same stage
        ? validate_flags (l, stgbeg, beg + 1, end, modf)
        //next stage
        : l.has_repeated_chars (stgbeg, beg)
          ? false
          : validate_width (l, beg, beg, end, literal ("0123456789"))
      : l.has_repeated_chars (stgbeg, beg)
        ? false
        : true;
  }
  //----------------------------------------------------------------------------
};
//------------------------------------------------------------------------------
struct floating_point_format_validation {
public:
  //----------------------------------------------------------------------------
  static constexpr bool validate_format_modifiers (const literal& l)
  {
    return (l.size() == 0)
      ? true
      : validate_flags (l, 0, l.size(), literal (" #+-0"), 0);
  }
  //----------------------------------------------------------------------------
private:
  //----------------------------------------------------------------------------
  static constexpr bool validate_specifiers(
    const literal& l, int beg, int end, const literal& modf
    )
  {
    // same specifiers as the integers
    return integral_format_validation::validate_specifiers(
      l, beg, end, modf
      );
  }
  //----------------------------------------------------------------------------
  static constexpr bool validate_width_precision(
    const literal& l, int beg, int end, const literal& modf, int has_dot = 0
    )
  {
    return
      (beg < end)
      ? (modf.find (l[beg], 0, modf.size(), -1) != -1)
        //keep validating the same stage
        ? (has_dot == 0 || l[beg] != '.')
          ? validate_width_precision(
            l, beg + 1, end, modf, has_dot + (l[beg] == '.')
            )
          // duplicated dot
          : false
        //next stage
        : validate_specifiers (l, beg, end, literal ("fFeEgGaA"))
      : true;
  }
  //----------------------------------------------------------------------------
  static constexpr bool validate_flags(
    const literal& l, int beg, int end, const literal& modf, int stgbeg
    )
  {
    // this function validates the flags or jumps to the width/precision
    // modifiers
    return
      (beg < end)
      ? (modf.find (l[beg], 0, modf.size(), -1) != -1)
        //keep validating the same stage
        ? validate_flags (l, beg + 1, end, modf, stgbeg)
        //next stage
        : l.has_repeated_chars (stgbeg, beg)
          ? false
          : validate_width_precision (l, beg, end, literal (".0123456789"))
      : l.has_repeated_chars (stgbeg, beg)
        ? false
        : true;
  }
  //----------------------------------------------------------------------------
};
//------------------------------------------------------------------------------
// BUILTIN TYPES
//------------------------------------------------------------------------------
template <class T>
struct is_valid_builtin : public std::integral_constant<
  bool,
  std::is_arithmetic<remove_cvref_t<T> >::value &&
  !std::is_same<remove_cvref_t<T>, bool>::value
  >
{};
//------------------------------------------------------------------------------
template <class T>
struct builtin_type_id;

template <>
struct builtin_type_id<uint8_t> : public attach_type_id<malc_type_u8>
{};
template <>
struct builtin_type_id<int8_t> : public attach_type_id<malc_type_i8>
{};
template <>
struct builtin_type_id<uint16_t> : public attach_type_id<malc_type_u16>
{};
template <>
struct builtin_type_id<int16_t> : public attach_type_id<malc_type_i16>
{};
template <>
struct builtin_type_id<uint32_t> : public attach_type_id<malc_type_u32>
{};
template <>
struct builtin_type_id<int32_t> : public attach_type_id<malc_type_i32>
{};
template <>
struct builtin_type_id<uint64_t> : public attach_type_id<malc_type_u64>
{};
template <>
struct builtin_type_id<int64_t> : public attach_type_id<malc_type_i64>
{};
template <>
struct builtin_type_id<float> : public attach_type_id<malc_type_float>
{};
template <>
struct builtin_type_id<double> : public attach_type_id<malc_type_double>
{};
//------------------------------------------------------------------------------
template <class T>
struct builtin_type_base :
  public builtin_type_id<T>,
  public attach_serialization_types<T>,
  public no_serializable_conversion<T>,
  public sizeof_wire_size<T>,
  public serialization_from_c_impl<T>
{};
//------------------------------------------------------------------------------
#if MALC_BUILTIN_COMPRESSION == 1
//------------------------------------------------------------------------------
template <>
struct builtin_type_base<int32_t> :
  public reuse_all_possible_from_c_impl<
    int32_t, builtin_type_id<int32_t>::type_id
    >
{};

template <>
struct builtin_type_base<uint32_t> :
  public reuse_all_possible_from_c_impl<
    uint32_t, builtin_type_id<uint32_t>::type_id
    >
{};

template <>
struct builtin_type_base<int64_t> :
  public reuse_all_possible_from_c_impl<
    int64_t, builtin_type_id<int64_t>::type_id
    >
{};

template <>
struct builtin_type_base<uint64_t> :
  public reuse_all_possible_from_c_impl<
    uint64_t, builtin_type_id<uint64_t>::type_id
    >
{};
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
// Definition of logged types
//------------------------------------------------------------------------------
template <class T>
struct logged_type<
  T,
  typename std::enable_if<is_valid_builtin<T>::value>::type
  > :
  public builtin_type_base<T>,
  public std::conditional<
    std::is_floating_point<T>::value,
    floating_point_format_validation,
    integral_format_validation
    >::type
{};
//------------------------------------------------------------------------------
template <>
struct logged_type<malc_strcp> :
  public attach_type_id<malc_type_strcp>,
  public attach_serialization_types<malc_strcp>,
  public no_serializable_conversion<malc_strcp>,
  public wire_size_from_c_impl<malc_strcp>,
  public serialization_from_c_impl<malc_strcp>,
  public default_format_validation
{};
//------------------------------------------------------------------------------
template <>
struct logged_type<malc_memcp> :
  public attach_type_id<malc_type_memcp>,
  public attach_serialization_types<malc_memcp>,
  public no_serializable_conversion<malc_memcp>,
  public wire_size_from_c_impl<malc_memcp>,
  public serialization_from_c_impl<malc_memcp>,
  public default_format_validation
{};
//------------------------------------------------------------------------------
// The types below store pointers. Reusing the C implementation.
template <>
struct logged_type<void*> :
  public reuse_all_possible_from_c_impl<void const*, malc_type_ptr>,
  public default_format_validation
{};
//------------------------------------------------------------------------------
template <>
struct logged_type<malc_lit> :
  public reuse_all_possible_from_c_impl<malc_lit, malc_type_lit>,
  public default_format_validation
{};
//------------------------------------------------------------------------------
template <>
struct logged_type<malc_strref> :
  public reuse_all_possible_from_c_impl<malc_strref, malc_type_strref>,
  public default_format_validation
{};
//------------------------------------------------------------------------------
template <>
struct logged_type<malc_memref> :
  public reuse_all_possible_from_c_impl<malc_memref, malc_type_memref>,
  public default_format_validation
{};
//------------------------------------------------------------------------------
template <>
struct logged_type<malc_refdtor> :
  public reuse_all_possible_from_c_impl<malc_refdtor, malc_type_refdtor>,
  public default_format_validation
{};
//------------------------------------------------------------------------------
// Special case for logging rvalues to std::string
//------------------------------------------------------------------------------
}} // namespace detail { namespace serialization {

static inline detail::serialization::malc_strcp strcp (std::string const& s);

namespace detail { namespace serialization {
//------------------------------------------------------------------------------
struct std_string_rvalue {
  std::string s;
};
//------------------------------------------------------------------------------
template <>
struct logged_type<std_string_rvalue> :
  public attach_type_id<malc_type_strcp>,
  public default_format_validation
{
  //----------------------------------------------------------------------------
  using external_type = std_string_rvalue;
  //----------------------------------------------------------------------------
  struct serializable_type {
    serializable_type (std::string&& v)
    {
      s = std::move (v);
      sdata = strcp (s);
    }
    serializable_type (serializable_type&& v) :
      serializable_type (std::move (v.s))
    {}

    std::string s;
    malc_strcp  sdata;
  };
  //----------------------------------------------------------------------------
  static inline serializable_type to_serializable (external_type& v)
  {
    return serializable_type (std::move (v.s));
  }

  static inline serializable_type to_serializable (external_type&& v)
  {
    return serializable_type (std::move (v.s));
  }
  //----------------------------------------------------------------------------
  static inline std::size_t wire_size (serializable_type const& v)
  {
    return logged_type<malc_strcp>::wire_size (v.sdata);
  }
  //----------------------------------------------------------------------------
  static inline void serialize (malc_serializer& s, serializable_type const& v)
  {
    return logged_type<malc_strcp>::serialize (s, v.sdata);
  }
  //----------------------------------------------------------------------------
};
//------------------------------------------------------------------------------
static constexpr uint16_t basic_types_compress_count (uint8_t type_id)
{
  return static_cast<uint16_t> (malc_total_compress_count (type_id));
}
//------------------------------------------------------------------------------
}}} // namespace malcpp { namespace detail { namespace serialization {

#endif // __MALC_CPP11_BASIC_TYPES_HPP__
