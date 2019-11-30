#ifndef __MALC_SERIALIZATION_HPP__
#define __MALC_SERIALIZATION_HPP__

#include <malc/common.h>
#include <malc/impl/common.h>
#include <malc/impl/serialization.h>

#include <malc/impl/metaprogramming_common.hpp>

/*----------------------------------------------------------------------------*/
namespace malcpp { namespace detail { namespace serialization {
/*----------------------------------------------------------------------------*/
template <typename T>
struct type_base {
  static inline bl_uword size (T v)      { return sizeof v; }
  static inline T        transform (T v) { return v; }
};

template<typename T, typename enable = void>
struct type {};

template<>
struct type<float> : public type_base<float> {
  static const char id = malc_type_float;
};

template<>
struct type<double> : public type_base<double> {
  static const char id = malc_type_double;
};

template<>
struct type<bl_i8> :public type_base<bl_i8> {
  static const char id = malc_type_i8;
};

template<>
struct type<bl_u8> : public type_base<bl_u8> {
  static const char id = malc_type_u8;
};

template<> struct type<bl_i16> : public type_base<bl_i16> {
  static const char id = malc_type_i16;
};

template<> struct type<bl_u16> : public type_base<bl_u16> {
    static const char id = malc_type_u16;
};
/*----------------------------------------------------------------------------*/
#if MALC_BUILTIN_COMPRESSION == 0
/*----------------------------------------------------------------------------*/
template<> struct type<bl_i32> : public type_base<bl_i32> {
  static const char id = malc_type_i32;
};

template<> struct type<bl_u32> : public type_base<bl_u32> {
  static const char id = malc_type_u32;
};

template<> struct type<bl_i64> : public type_base<bl_i64> {
  static const char id = malc_type_i64;
};

template<> struct type<bl_u64> : public type_base<bl_u64> {
  static const char id = malc_type_u64;
};
/*----------------------------------------------------------------------------*/
#else /* #if MALC_BUILTIN_COMPRESSION == 0 */
/*----------------------------------------------------------------------------*/
// as the compression the only that does is to look for trailing zeros, the
// integers under 4 bytes and floating point values are omitted. This
// compression works best if a lot of 64-bit integers with small values are
// passed.
template<>
struct type<bl_i32> {
  static const char id = malc_type_i32;
  static inline malc_compressed_32 transform (bl_i32 v)
  {
    return malc_get_compressed_i32 (v);
  }
};

template<>
struct type<bl_u32> {
  static const char id = malc_type_u32;
  static inline malc_compressed_32 transform (bl_u32 v)
  {
    return malc_get_compressed_u32 (v);
  }
};

template<>
struct type<bl_i64> {
  static const char id = malc_type_i64;
  static inline malc_compressed_64 transform (bl_i64 v)
  {
    return malc_get_compressed_i64 (v);
  }
};

template<>
struct type<bl_u64> {
  static const char id = malc_type_u64;
  static inline malc_compressed_64 transform (bl_u64 v)
  {
    return malc_get_compressed_u64 (v);
  }
};
/*----------------------------------------------------------------------------*/
#endif /* #else  #if MALC_BUILTIN_COMPRESSION == 0 */
/*----------------------------------------------------------------------------*/
#if MALC_PTR_COMPRESSION == 0
/*----------------------------------------------------------------------------*/
template<> struct type<void*> : public type_base<void*> {
    static const char id = malc_type_ptr;
};

template<> struct type<malc_lit> : public type_base<malc_lit> {
    static const char id = malc_type_lit;
};

template<> struct type<malc_strref> {
  static const char id = malc_type_strref;
  static inline malc_strref transform (malc_strref v) { return v; }
  static inline bl_uword size (malc_strref v)
  {
    return bl_sizeof_member (malc_strref, str) +
           bl_sizeof_member (malc_strref, len);
  }
};

template<> struct type<malc_memref> {
  static const char id = malc_type_memref;
  static inline malc_memref transform (malc_memref v) { return v; }
  static inline bl_uword size (malc_memref v)
  {
    return bl_sizeof_member (malc_memref, mem) +
           bl_sizeof_member (malc_memref, size);
  }
};

template<>
struct type<malc_refdtor> {
  static const char id = malc_type_refdtor;
  static inline malc_refdtor transform (malc_refdtor v) { return v; }
  static inline bl_uword size (malc_refdtor v)
  {
    return bl_sizeof_member (malc_refdtor, func) +
           bl_sizeof_member (malc_refdtor, context);
  }
};
/*----------------------------------------------------------------------------*/
#else // #if MALC_PTR_COMPRESSION == 0
/*----------------------------------------------------------------------------*/
template<>
struct type<void*> {
  static const char id = malc_type_ptr;
  static inline malc_compressed_ptr transform (void* v)
  {
    return malc_get_compressed_ptr (v);
  }
};

template<>
struct type<malc_lit> {
  static const char id = malc_type_lit;
  static inline malc_compressed_ptr transform (malc_lit v)
  {
    return malc_get_compressed_ptr ((void*) v.lit);
  }
};

template<>
struct type<malc_strref> {
  static const char id = malc_type_strref;
  static inline malc_compressed_ref transform (malc_strref v)
  {
    malc_compressed_ref r;
    r.ref  = malc_get_compressed_ptr ((void*) v.str);
    r.size = v.len;
    return r;
  }
};

template<>
struct type<malc_memref> {
  static const char id = malc_type_memref;
  static inline malc_compressed_ref transform (malc_memref v)
  {
    malc_compressed_ref r;
    r.ref  = malc_get_compressed_ptr ((void*) v.mem);
    r.size = v.size;
    return r;
  }
};

template<>
struct type<malc_refdtor> {
  static const char id = malc_type_refdtor;
  static inline malc_compressed_refdtor transform (malc_refdtor v)
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
struct type<const void*> : public type<void*> {};

template<>
struct type<volatile void*> : public type<void*> {};

template<>
struct type<const volatile void*> : public type<void*> {};

template<>
struct type<void* const> : public type<void*> {};

template<>
struct type<const void* const> : public type<void*> {};

template<>
struct type<volatile void* const> : public type<void*> {};

template<>
struct type<const volatile void* const> : public type<void*> {};

template<>
struct type<malc_strcp> {
  static const char id  = malc_type_strcp;
  static inline malc_strcp transform (malc_strcp v) { return v; }
  static inline bl_uword size (malc_strcp v)
  {
    return bl_sizeof_member (malc_strcp, len) + v.len;
  }
};

template<>
struct type<malc_memcp> {
  static const char id = malc_type_memcp;
  static inline malc_memcp transform (malc_memcp v) { return v; }
  static inline bl_uword size (malc_memcp v)
  {
    return bl_sizeof_member (malc_memcp, size) + v.size;
  }
};
/*----------------------------------------------------------------------------*/
#if MALC_COMPRESSION == 1
/*----------------------------------------------------------------------------*/
template<>
struct type<malc_compressed_32> {
  static inline bl_uword size (malc_compressed_32 v)
  {
    return malc_compressed_get_size (v.format_nibble);
  }
};

template<>
struct type<malc_compressed_64> {
  static inline bl_uword size (malc_compressed_64 v)
  {
    return malc_compressed_get_size (v.format_nibble);
  }
};

template<>
struct type<malc_compressed_ref> {
  static inline bl_uword size (malc_compressed_ref v)
  {
    return bl_sizeof_member (malc_compressed_ref, size) +
           malc_compressed_get_size (v.ref.format_nibble);
  }
};

template<>
struct type<malc_compressed_refdtor> {
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
