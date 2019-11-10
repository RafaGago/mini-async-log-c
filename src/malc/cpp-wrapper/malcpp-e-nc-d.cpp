#ifdef __cplusplus

#include <malc/cpp-wrapper/malcpp.hpp>

/* adding all template variations one by one, so at least the linker has it
easy to discard unused versions when static linking */

template class malcpp::malcpp<true, false, true>;

#endif /* __cplusplus */