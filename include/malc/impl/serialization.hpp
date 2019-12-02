#ifndef __MALC_SERIALIZATION_HPP__
#define __MALC_SERIALIZATION_HPP__

#include <malc/common.h>
#include <malc/impl/common.h>
#include <malc/impl/serialization.h>

#include <malc/impl/metaprogramming_common.hpp>
/*------------------------------------------------------------------------------
The serialization is done through "sertype" class specializations.

"sertype" contains:

-"id": A numeric value to identify the type.
-"to_serialization_type": A function to convert the type to a serialization
  friendly format.
-"size": A function to calculate the serialization friendly type's size on the
 "wire".

The sertype specializations that return the same type that they get as parameter
on the  "to_serialization_type" function contain all the previous three
attributes.

The types that return a different type that they get on "to_serialization_type"
contain "id" and "to_serialization_type" on the "sertype" specialization for the
user-facing type and the "size" function on the sertype specialization for the
serializable type.

All the serializable types are serialized by calling "malc_serialize".
malc_serialize is reused from the C implementation, so it isn't present here.
------------------------------------------------------------------------------*/
namespace malcpp { namespace detail { namespace serialization {
/*----------------------------------------------------------------------------*/
template<typename T, typename enable = void>
struct sertype;

template<typename T, typename enable>
struct sertype {
  static constexpr char id = malc_type_error;
};

template <typename T, char serid>
struct trivial_sertype {
  static constexpr char id = serid;
  static inline bl_uword size (T v)                  { return sizeof v; }
  static inline T        to_serialization_type (T v) { return v; }
};

template<>
struct sertype<float> : public trivial_sertype<float, malc_type_float> {};

template<>
struct sertype<double> : public trivial_sertype<double, malc_type_double> {};

template<>
struct sertype<bl_i8> : public trivial_sertype<bl_i8, malc_type_i8> {};

template<>
struct sertype<bl_u8> : public trivial_sertype<bl_i8, malc_type_u8> {};

template<>
struct sertype<bl_i16> : public trivial_sertype<bl_i16, malc_type_i16> {};

template<>
struct sertype<bl_u16> : public trivial_sertype<bl_i16, malc_type_u16> {};
/*----------------------------------------------------------------------------*/
#if MALC_BUILTIN_COMPRESSION == 0
/*----------------------------------------------------------------------------*/
template<>
struct sertype<bl_i32> : public trivial_sertype<bl_i32, malc_type_i32> {};

template<>
struct sertype<bl_u32> : public trivial_sertype<bl_i32, malc_type_u32> {};

template<>
struct sertype<bl_i64> : public trivial_sertype<bl_i64, malc_type_i64> {};

template<>
struct sertype<bl_u64> : public trivial_sertype<bl_i64, malc_type_u64> {};
/*----------------------------------------------------------------------------*/
#else /* #if MALC_BUILTIN_COMPRESSION == 0 */
/*----------------------------------------------------------------------------*/
// as the compression the only that does is to look for trailing zeros, the
// integers under 4 bytes and floating point values are omitted. This
// compression works best if a lot of 64-bit integers with small values are
// passed.
template<>
struct sertype<bl_i32> {
  static constexpr char id = malc_type_i32;
  static inline malc_compressed_32 to_serialization_type (bl_i32 v)
  {
    return malc_get_compressed_i32 (v);
  }
};

template<>
struct sertype<bl_u32> {
  static constexpr char id = malc_type_u32;
  static inline malc_compressed_32 to_serialization_type (bl_u32 v)
  {
    return malc_get_compressed_u32 (v);
  }
};

template<>
struct sertype<bl_i64> {
  static constexpr char id = malc_type_i64;
  static inline malc_compressed_64 to_serialization_type (bl_i64 v)
  {
    return malc_get_compressed_i64 (v);
  }
};

template<>
struct sertype<bl_u64> {
  static constexpr char id = malc_type_u64;
  static inline malc_compressed_64 to_serialization_type (bl_u64 v)
  {
    return malc_get_compressed_u64 (v);
  }
};
/*----------------------------------------------------------------------------*/
#endif /* #else  #if MALC_BUILTIN_COMPRESSION == 0 */
/*----------------------------------------------------------------------------*/
#if MALC_PTR_COMPRESSION == 0
/*----------------------------------------------------------------------------*/
template <class T>
struct sertype<
  T*,
  typename std::enable_if<std::is_same<remove_cvref_t<T*>, void*>::value>::type
  > : public trivial_sertype<void*, malc_type_ptr> {};

template<>
struct sertype<malc_lit> : public trivial_sertype<malc_lit, malc_type_lit> {};

template<> struct sertype<malc_strref> {
  static constexpr char id = malc_type_strref;
  static inline malc_strref to_serialization_type (malc_strref v) { return v; }
  static inline bl_uword size (malc_strref v)
  {
    return bl_sizeof_member (malc_strref, str) +
           bl_sizeof_member (malc_strref, len);
  }
};

template<> struct sertype<malc_memref> {
  static constexpr char id = malc_type_memref;
  static inline malc_memref to_serialization_type (malc_memref v) { return v; }
  static inline bl_uword size (malc_memref v)
  {
    return bl_sizeof_member (malc_memref, mem) +
           bl_sizeof_member (malc_memref, size);
  }
};

template<>
struct sertype<malc_refdtor> {
  static constexpr char id = malc_type_refdtor;
  static inline malc_refdtor to_serialization_type (malc_refdtor v) { return v; }
  static inline bl_uword size (malc_refdtor v)
  {
    return bl_sizeof_member (malc_refdtor, func) +
           bl_sizeof_member (malc_refdtor, context);
  }
};
/*----------------------------------------------------------------------------*/
#else // #if MALC_PTR_COMPRESSION == 0
/*----------------------------------------------------------------------------*/
template <class T>
struct sertype<
  T*,
  std::enable_if<std::is_same <remove_cvref_t<T>, void*>::type>
  > {
  static constexpr char id = malc_type_ptr;
  static inline malc_compressed_ptr to_serialization_type (void* v)
  {
    return malc_get_compressed_ptr (v);
  };
};

template<>
struct sertype<malc_lit> {
  static constexpr char id = malc_type_lit;
  static inline malc_compressed_ptr to_serialization_type (malc_lit v)
  {
    return malc_get_compressed_ptr ((void*) v.lit);
  }
};

template<>
struct sertype<malc_strref> {
  static constexpr char id = malc_type_strref;
  static inline malc_compressed_ref to_serialization_type (malc_strref v)
  {
    malc_compressed_ref r;
    r.ref  = malc_get_compressed_ptr ((void*) v.str);
    r.size = v.len;
    return r;
  }
};

template<>
struct sertype<malc_memref> {
  static constexpr char id = malc_type_memref;
  static inline malc_compressed_ref to_serialization_type (malc_memref v)
  {
    malc_compressed_ref r;
    r.ref  = malc_get_compressed_ptr ((void*) v.mem);
    r.size = v.size;
    return r;
  }
};

template<>
struct sertype<malc_refdtor> {
  static constexpr char id = malc_type_refdtor;
  static inline malc_compressed_refdtor to_serialization_type (malc_refdtor v)
  {
    malc_compressed_refdtor r;
    r.func    = malc_get_compressed_ptr ((void*) v.func);
    r.context = malc_get_compressed_ptr ((void*) v.context);
    return r;
  }
};
/*----------------------------------------------------------------------------*/
#endif // #if MALC_PTR_COMPRESSION == 0
/*----------------------------------------------------------------------------*/
template<>
struct sertype<malc_strcp> {
  static constexpr char id  = malc_type_strcp;
  static inline malc_strcp to_serialization_type (malc_strcp v) { return v; }
  static inline bl_uword size (malc_strcp v)
  {
    return bl_sizeof_member (malc_strcp, len) + v.len;
  }
};

template<>
struct sertype<malc_memcp> {
  static constexpr char id = malc_type_memcp;
  static inline malc_memcp to_serialization_type (malc_memcp v) { return v; }
  static inline bl_uword size (malc_memcp v)
  {
    return bl_sizeof_member (malc_memcp, size) + v.size;
  }
};
/*----------------------------------------------------------------------------*/
#if MALC_COMPRESSION == 1
/*----------------------------------------------------------------------------*/
template<>
struct sertype<malc_compressed_32> {
  static inline bl_uword size (malc_compressed_32 v)
  {
    return malc_compressed_get_size (v.format_nibble);
  }
};

template<>
struct sertype<malc_compressed_64> {
  static inline bl_uword size (malc_compressed_64 v)
  {
    return malc_compressed_get_size (v.format_nibble);
  }
};

template<>
struct sertype<malc_compressed_ref> {
  static inline bl_uword size (malc_compressed_ref v)
  {
    return bl_sizeof_member (malc_compressed_ref, size) +
           malc_compressed_get_size (v.ref.format_nibble);
  }
};

template<>
struct sertype<malc_compressed_refdtor> {
  static inline bl_uword size (malc_compressed_refdtor v)
  {
    return malc_compressed_get_size (v.func.format_nibble) +
           malc_compressed_get_size (v.context.format_nibble);
  }
};
/*----------------------------------------------------------------------------*/
#endif /* #if MALC_COMPRESSION == 1 */

//------------------------------------------------------------------------------
template <class T>
struct is_malc_refvalue {
  static constexpr bool value =
    std::is_same<remove_cvref_t<T>, serialization::malc_memref>::value |
    std::is_same<remove_cvref_t<T>, serialization::malc_strref>::value;
};

}}} // namespace malcpp { namespace detail { namespace serialization {

#endif // __MALC_SERIALIZATION_HPP__
