#include <bl/base/error.h>
#include <malc/malc_lean.hpp>

template <int v>
class trigger_compile_errors {};

#define fmttest(...) \
[]() { \
  using argtypelist = decltype (malcpp::detail::make_typelist( \
        bl_pp_vargs_ignore_first (__VA_ARGS__) /*1st arg = format str*/ \
        )); \
  ::malcpp::detail::fmt::static_arg_validation< \
    ::malcpp::detail::fmt::refvalues::validate<argtypelist>(), \
    ::malcpp::detail::fmt::format_string::validate<argtypelist>( \
        bl_pp_vargs_first (__VA_ARGS__) /*1st arg = format str*/\
        ) \
    >(); \
}()

void tests();

int main()
{
  tests();
  return 0;
}