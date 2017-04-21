#ifndef __MALC_LIBEXPORT_H__
#define __MALC_LIBEXPORT_H__

#include <bl/base/platform.h>

#define MALC_EXPORT

#if defined (BL_GCC)
  #if BL_GCC >= BL_GCC_VER (4, 0, 0)
    #if !defined (MALC_PRIVATE_SYMS)
      #undef MALC_EXPORT
      #define MALC_EXPORT __attribute__ ((visibility ("default")))
    #endif
  #endif

#elif defined (MALC_MSC)
  #if (defined (MALC_SHAREDLIB_COMPILATION) &&\
    !defined (MALC_SHAREDLIB_USING_DEF)) ||\
    (defined (MALC_ALL_LIBS_SHAREDLIB_COMPILATION) &&\
    !defined (MALC_ALL_LIBS_SHAREDLIB_USING_DEF))

    #undef MALC_EXPORT
    #define MALC_EXPORT __declspec (dllexport)

  #elif (defined (MALC_SHAREDLIB) &&\
    !defined (MALC_SHAREDLIB_USING_DEF)) ||\
    (defined (MALC_ALL_LIBS_SHAREDLIB) &&\
    !defined (MALC_ALL_LIBS_SHAREDLIB_USING_DEF))

    #undef MALC_EXPORT
    #define MALC_EXPORT __declspec (dllimport)

   #endif
#endif

#endif /* __MALC_LIBEXPORT_H__ */
