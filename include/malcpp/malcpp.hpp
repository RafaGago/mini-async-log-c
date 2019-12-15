#ifndef __MALC_HPP__
#define __MALC_HPP__

// Include this file to use all malc's feature set.

/*------------------------------------------------------------------------------
DOCUMENTATION: See "malcpp/impl/malc_base.hpp"

NOTICE: When MALC_LEAN == 0 you can log some objects by reference.

MALC_LEAN == 0 enables smart (shared/weak/unique) pointers to:

-std::string:
-std::vector: To builtin arithmetic types except bool. The arithmetic types
 accept the same format string modifiers as the contained type does.

The instances contained on the smart pointers are assumed to not be modified
after being passed to malc. Data races and thread safety issues caused by
modifications on the object after logging aren't malc's resposibility.

MALC_LEAN == 0 also allows logging objects using std::ostreams. The objects
can be passed by value or wrapped in a smart pointer.

If you don't need these, include "malc_lean.hpp"; it's more lightweight.
------------------------------------------------------------------------------*/
#define MALCPP_LEAN 0
#include <malcpp/impl/malc_base.hpp>
#undef MALCPP_LEAN

#endif /* __MALC_HPP__ */