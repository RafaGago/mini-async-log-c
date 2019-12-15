#ifdef __cplusplus

#include <malcpp/malcpp-template.hpp>

/* adding all template variations one by one, so at least the linker has it
easy to discard unused versions when static linking */

template class malcpp::malcpp<true, true, true>;

#endif /* __cplusplus */