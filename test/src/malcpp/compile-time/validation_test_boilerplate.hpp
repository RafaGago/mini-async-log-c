#include <bl/base/error.h>
#include <malc/impl/cpp11/cpp11.hpp>

template <int v>
class trigger_compile_errors {};


//TODO: microsoft preprocessor will be special...<
#define malc_pp_vargs_first(a, ...) a

#define malc_pp_vargs_ignore_first(a, ...) __VA_ARGS__

#define valtest(...) \
  trigger_compile_errors< \
    malcpp::detail::fmt::format_string::validate< \
      decltype (malcpp::detail::fmt::make_typelist( \
        malc_pp_vargs_ignore_first (__VA_ARGS__) \
        )) \
      > (malc_pp_vargs_first (__VA_ARGS__)) \
    >()

void tests();

int main()
{
  tests();
  return 0;
}