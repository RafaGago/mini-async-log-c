#ifndef __MALC_SERIALIZATION_HPP__
#define __MALC_SERIALIZATION_HPP__

#ifndef MALC_COMMON_NAMESPACED
#define MALC_COMMON_NAMESPACED 1
#endif

#include <type_traits>

#include <malc/common.h>
#include <malc/impl/common.h>
#include <malc/impl/serialization.h>

#include <malc/impl/metaprogramming_common.hpp>

namespace malcpp { namespace detail { namespace serialization {
/*------------------------------------------------------------------------------
logged_type:

The serialization is done through "logged_type" class specializations.

"logged_type" contains:

*"static constexpr char id":

  A constexpr numeric value to identify the type when it is type erased at
  serialization time

*"external_type": The type that the user passes. Matches the specialization type

  on "logged_type"

*"serializable_type": A type that stores "external_type" + additional data that

 is the result of some process at the producer side. After this process the data
 is ready for serialization.

*static [constexpr|inline] serializable_type to_serializable (external_type v)

  Converts from an external type to a serializable type.

*static [constexpr|inline] std::size_t wire_size (serializable_type v)

  Get the serialized (wire) size.

*static inline void serialize (malc_serializer& s, serializable_type v)

  Serializes the type.
------------------------------------------------------------------------------*/
template <class T, class enable = void>
struct logged_type {
  static constexpr char id = malc_type_error;
};
//------------------------------------------------------------------------------
template <class T>
using get_logged_type = logged_type<remove_cvref_t<T> >;
//------------------------------------------------------------------------------
struct default_format_validation {
public:
  static constexpr bool validate_format_modifiers (const literal& l)
  {
    return l.size() == 0;
  }
};
//------------------------------------------------------------------------------
template <char id>
struct attach_type_id {
  static constexpr char type_id = id;
};
//------------------------------------------------------------------------------
template <class external, class serializable = external>
struct attach_serialization_types {
  using serializable_type = serializable;
  using external_type     = external;
};
//------------------------------------------------------------------------------
template <class T>
struct no_serializable_conversion {
  static constexpr T to_serializable (T v)
  {
    return v;
  }
};
//------------------------------------------------------------------------------
template <class T>
struct serializable_conversion_and_types_from_c_impl {
  using serializable_type = decltype (malc_transform (*((T*) nullptr)));
  using external_type     = T;

  static inline serializable_type to_serializable (T v)
  {
    return malc_transform (v);
  }
};
//------------------------------------------------------------------------------
template <class T>
struct sizeof_wire_size {
  static constexpr std::size_t wire_size (T v)
  {
    return sizeof v;
  }
};
//------------------------------------------------------------------------------
template <class T>
struct wire_size_from_c_impl {
  static inline std::size_t wire_size (T v)
  {
    return malc_size (v);
  }
};
//------------------------------------------------------------------------------
template <class T>
struct serialization_from_c_impl {
  static inline void serialize (malc_serializer& s, T v)
  {
    malc_serialize (&s, v);
  }
};
//------------------------------------------------------------------------------
template <class T>
struct is_malc_refvalue {
  static constexpr bool value =
    std::is_same<remove_cvref_t<T>, malc_memref>::value ||
    std::is_same<remove_cvref_t<T>, malc_strref>::value;
};
//------------------------------------------------------------------------------
}}} // namespace malcpp { namespace detail { namespace serialization {

#endif // __MALC_SERIALIZATION_HPP__
