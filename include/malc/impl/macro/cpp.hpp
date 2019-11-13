#ifndef __MALC_MACRO_CPP_H__
#define __MALC_MACRO_CPP_H__

/*
provides :
-malc_get_type_code
-malc_type_size
-malc_make_var_from_expression

By using C++11. This is using the C interface by using C++
*/

#include <type_traits>

#include <malc/common.h>
#include <malc/impl/common.h>

template <typename T>
struct malc_type_traits_base {
  static inline bl_uword size      (T v) { return sizeof v; }
  static inline T     transform (T v) { return v; }
};
template<typename T> struct malc_type_traits {};

template<> struct malc_type_traits<float> :
  public malc_type_traits_base<float> {
    static const char  code = malc_type_float;
};
template<> struct malc_type_traits<double> :
  public malc_type_traits_base<double> {
    static const char  code = malc_type_double;
};
template<> struct malc_type_traits<bl_i8> :
  public malc_type_traits_base<bl_i8> {
    static const char  code = malc_type_i8;
};
template<> struct malc_type_traits<bl_u8> :
  public malc_type_traits_base<bl_u8> {
    static const char  code = malc_type_u8;
};
template<> struct malc_type_traits<bl_i16> :
  public malc_type_traits_base<bl_i16> {
    static const char  code = malc_type_i16;
};
template<> struct malc_type_traits<bl_u16> :
  public malc_type_traits_base<bl_u16> {
    static const char  code = malc_type_u16;
};

#if MALC_BUILTIN_COMPRESSION == 0
  template<> struct malc_type_traits<bl_i32> :
    public malc_type_traits_base<bl_i32> {
      static const char  code = malc_type_i32;
  };
  template<> struct malc_type_traits<bl_u32> :
    public malc_type_traits_base<bl_u32> {
      static const char  code = malc_type_u32;
  };
  template<> struct malc_type_traits<bl_i64> :
    public malc_type_traits_base<bl_i64> {
      static const char  code = malc_type_i64;
  };
  template<> struct malc_type_traits<bl_u64> :
    public malc_type_traits_base<bl_u64> {
      static const char  code = malc_type_u64;
  };
#else /* #if MALC_BUILTIN_COMPRESSION == 0 */
  template<> struct malc_type_traits<bl_i32> {
    static const char  code = malc_type_i32;
    static inline malc_compressed_32 transform (bl_i32 v)
    {
      return malc_get_compressed_i32 (v);
    }
  };
  template<> struct malc_type_traits<bl_u32> {
    static const char  code = malc_type_u32;
    static inline malc_compressed_32 transform (bl_u32 v)
    {
      return malc_get_compressed_u32 (v);
    }
  };
  template<> struct malc_type_traits<bl_i64> {
    static const char  code = malc_type_i64;
    static inline malc_compressed_64 transform (bl_i64 v)
    {
      return malc_get_compressed_i64 (v);
    }
  };
  template<> struct malc_type_traits<bl_u64> {
    static const char  code = malc_type_u64;
    static inline malc_compressed_64 transform (bl_u64 v)
    {
      return malc_get_compressed_u64 (v);
    }
  };
#endif /* #else  #if MALC_BUILTIN_COMPRESSION == 0 */

#if MALC_PTR_COMPRESSION == 0
  template<> struct malc_type_traits<void*> :
    public malc_type_traits_base<void*> {
      static const char code = malc_type_ptr;
  };
  template<> struct malc_type_traits<malc_lit> :
    public malc_type_traits_base<malc_lit> {
      static const char code = malc_type_lit;
  };
  template<> struct malc_type_traits<malc_strref> {
    static const char code = malc_type_strref;
    static inline malc_strref transform (malc_strref v) { return v; }
    static inline bl_uword size (malc_strref v)
    {
      return bl_sizeof_member (malc_strref, str) +
             bl_sizeof_member (malc_strref, len);
    }
  };
  template<> struct malc_type_traits<malc_memref> {
    static const char code = malc_type_memref;
    static inline malc_memref transform (malc_memref v) { return v; }
    static inline bl_uword size (malc_memref v)
    {
      return bl_sizeof_member (malc_memref, mem) +
             bl_sizeof_member (malc_memref, size);
    }
  };
  template<> struct malc_type_traits<malc_refdtor> {
    static const char code = malc_type_refdtor;
    static inline malc_refdtor transform (malc_refdtor v) { return v; }
    static inline bl_uword size (malc_refdtor v)
    {
      return bl_sizeof_member (malc_refdtor, func) +
             bl_sizeof_member (malc_refdtor, context);
    }
  };
#else
  template<> struct malc_type_traits<void*> {
    static const char code = malc_type_ptr;
    static inline malc_compressed_ptr transform (void* v)
    {
      return malc_get_compressed_ptr (v);
    }
  };
  template<> struct malc_type_traits<malc_lit> {
    static const char code = malc_type_lit;
    static inline malc_compressed_ptr transform (malc_lit v)
    {
      return malc_get_compressed_ptr ((void*) v.lit);
    }
  };
  template<> struct malc_type_traits<malc_strref> {
    static const char code = malc_type_strref;
    static inline malc_compressed_ref transform (malc_strref v)
    {
      malc_compressed_ref r;
      r.ref  = malc_get_compressed_ptr ((void*) v.str);
      r.size = v.len;
      return r;
    }
  };
  template<> struct malc_type_traits<malc_memref> {
    static const char code = malc_type_memref;
    static inline malc_compressed_ref transform (malc_memref v)
    {
      malc_compressed_ref r;
      r.ref  = malc_get_compressed_ptr ((void*) v.mem);
      r.size = v.size;
      return r;
    }
  };
  template<> struct malc_type_traits<malc_refdtor> {
    static const char code = malc_type_refdtor;
    static inline malc_compressed_refdtor transform (malc_refdtor v)
    {
      malc_compressed_refdtor r;
      r.func    = malc_get_compressed_ptr ((void*) v.func);
      r.context = malc_get_compressed_ptr ((void*) v.context);
      return r;
    }
  };
#endif

template<> struct malc_type_traits<const void*> :
  public malc_type_traits<void*> {};

template<> struct malc_type_traits<volatile void*> :
  public malc_type_traits<void*> {};

template<> struct malc_type_traits<const volatile void*> :
  public malc_type_traits<void*> {};

template<> struct malc_type_traits<void* const> :
  public malc_type_traits<void*> {};

template<> struct malc_type_traits<const void* const> :
  public malc_type_traits<void*> {};

template<> struct malc_type_traits<volatile void* const> :
  public malc_type_traits<void*> {};

template<> struct malc_type_traits<const volatile void* const> :
  public malc_type_traits<void*> {};

template<> struct malc_type_traits<malc_strcp> {
  static const char code  = malc_type_strcp;
  static inline malc_strcp transform (malc_strcp v) { return v; }
  static inline bl_uword size (malc_strcp v)
  {
    return bl_sizeof_member (malc_strcp, len) + v.len;
  }
};
template<> struct malc_type_traits<malc_memcp> {
  static const char  code = malc_type_memcp;
  static inline malc_memcp transform (malc_memcp v) { return v; }
  static inline bl_uword size (malc_memcp v)
  {
    return bl_sizeof_member (malc_memcp, size) + v.size;
  }
};

#if MALC_COMPRESSION == 1
template<> struct malc_type_traits<malc_compressed_32> {
  static inline bl_uword size (malc_compressed_32 v)
  {
    return malc_compressed_get_size (v.format_nibble);
  }
};
template<> struct malc_type_traits<malc_compressed_64> {
  static inline bl_uword size (malc_compressed_64 v)
  {
    return malc_compressed_get_size (v.format_nibble);
  }
};
template<> struct malc_type_traits<malc_compressed_ref> {
  static inline bl_uword size (malc_compressed_ref v)
  {
    return bl_sizeof_member (malc_compressed_ref, size) +
           malc_compressed_get_size (v.ref.format_nibble);
  }
};
template<> struct malc_type_traits<malc_compressed_refdtor> {
  static inline bl_uword size (malc_compressed_refdtor v)
  {
    return malc_compressed_get_size (v.func.format_nibble) +
           malc_compressed_get_size (v.context.format_nibble);
  }
};
#endif /* #if MALC_COMPRESSION == 1 */

#include <type_traits>

#define malc_get_type_code(expression)\
  malc_type_traits< \
    typename std::remove_cv< \
      typename std::remove_reference< \
        decltype (expression) \
        >::type \
      >::type \
    >::code

#define malc_type_size(expression)\
  malc_type_traits< \
    typename std::remove_cv< \
      typename std::remove_reference< \
        decltype (expression) \
        >::type \
      >::type \
    >::size (expression)

#define malc_make_var_from_expression(expression, name)\
  auto name = malc_type_traits< \
    typename std::remove_cv< \
      typename std::remove_reference< \
        decltype (expression) \
        >::type \
      >::type \
    >::transform (expression);

#endif /* __MALC_MACRO_CPP_H__ */
