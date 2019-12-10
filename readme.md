Description
===========

A C11/C++11 low-latency wait-free producer (when using Thread Local Storage)
asynchronous textual data logger with type-safe strings.

Based on the lessons learned on its older C++-only counterpart "mini-async-log".

Design
======

The life of a log message under this logger is.

At compile time:

-The validation of the format string is done at compile-time (C++ only).

-A "static const" data structure is created containing the format string, the
 severity, a code for each data type and the number of compressed datatypes (if
 enabled). All the previous data can be passed with just the overhead of one
 pointer.

At run time:

-A log call is made.

-The passed values are stored on the stack after being passed through a
 type-specific conversion function, which leaves them ready to serialize. The
 conversion function is just doing nothing on most cases.

-The required payload for serializing the log entry is computed, taking as input
 the transformed type on the step above. For simple cases it's just a compile
 time constant (sizeof (type)).

-Memory for an intrusive linked list node and all the serialization payload is
 requested in a single memory chunk.

-The pointer to the "static const" data structure and the payload data is
 serialized after the intrusive linked list node (unused at this point).

-The intrusive node is passed to the logging list.

-At some point the consumer thread fetches the node, deserializes it, applies
 formatting, forwards it to the log destinations/sinks and then frees the node
 memory.

Features
========

Common:

- Dual C/C++ library.

- Very high performance. I have not found yet a faster data logger from the
  consumer side. Even when comparing against non-textual ones. The benchmarks
  live on a separate project:

  https://github.com/RafaGago/logger-bench

- Various memory (log entry) sources: Thread Local Storage buffer, common
  bounded buffer (configurable to have one for each CPU) and custom log entry
  allocators (defaults to the heap's malloc/free).

- Can customize each and every allocation made. It's able to receive two
  allocators, one for allocating log entries and another to allocate the
  internals. You get a pointer to the allocator on each allocation/deallocation
  call made, so additional data/state can be attached after the structure.

- Type-safe format strings. On C11 achieved through C11 type-generic expressions
  and (unfortunately) brutal preprocessor abusing.

- Not a singleton. No hidden threads: the client application can run the
  logger's consumer loop from an existing (maybe shared for other purposes)
  thread if desired.

- Basic security features: Log entry rate limiting and newline removal.

- Extensible log destinations (sinks).

- Compile-time removable severities.

- Lazy evaluated parameters. If the log call is filtered out because of a low
  severity the parameters don't get evaluated (macros).

- Able to log strings/memory ranges by passing ownership, so the serialization
  overhead becomes a pointer. A destruction callback that frees resources
  has to be provided in this mode of operation.

- Decent test coverage.

C++ only:

- The instance control functions can be selected to return error codes, matching
  more or less the C interface, or to throw exceptions.

  This logger can be used in projects with non-throwing coding standards.
  Projects that are willing to use exceptions can write less error handling
  boilerplate by enabling the exception based interface. The logging functions
  are error code-based only, as they are free functions.

- Can log ostreamable types by value or wrapped in a shared pointer.

- Can log std::string, smart pointers to std::string and smart pointers to
  std::vector containing arithmetic types.

- It allows adding custom logging for additional data types.

- It almost doesn't leak any C function or type to the global namespace, and the
  few they do are prefixed.

Usage Quickstart
==================

Look at the examples folder.

Log format strings
==================

The format strings have this (common) bracket delimited form:

` "this is value 1: {} and this is value 2: {}" `

The opening brace '{' is escaped by doubling it. The close brace doesn't need
escaping.

Depending on the data type modifiers inside the brackets are accepted. Those try
to match own printf's modifiers.

As a reminder, "printf" format strings are composed like this:

` %[flags]\[width][.precision]\[length]specifier `

Where the valid chars for each field are (on C99):

```
-flags: #, 0, +, -
-width: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
-precision: ., 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, .*
-length: h, hh, l, ll, j, z, t, L
-specifiers: d, i, u , o, x, X, f, F, e, E, g, G, a, A, c, s, p, n, %
```

Malc autodetecs the datatypes passed to the log strings, so the "length"
modifiers and signedness "specifiers" are never required. Which specifiers can
be used on which data type is also restricted only to those that make sense.

Malc adds two non-numeric width specifiers 'W' and 'N':

-'W' represents the maximum digit count that the biggest/smallest value of an
 integer type can have.
-'N' represents the nibble count of a data type (useful to print hex).

On malc there are three width modifier groups [0-9], W and N. The three of them
are mutually exclusive. Only one of them can be specified, the result of not
doing so is undefined.

Next comes a summary of the valid modifiers for each type.

integrals (or std::vector of integrals)
---------------------------------------

-flags: `#, 0, +, -`
-width: `0, 1, 2, 3, 4, 5, 6, 7, 8, 9, W, N`
-specifiers: `o, x, X`

floating point (or std::vector of floating point)
-------------------------------------------------

-flags: `#, 0, +, -`
-width: `0, 1, 2, 3, 4, 5, 6, 7, 8, 9`
-precision: `., 0, 1, 2, 3, 4, 5, 6, 7, 8, 9`
-specifiers: `f, F, e, E, g, G, a, A`

others
------

Other types, like pointers, strings, etc. accept no modifiers.

examples
--------

`"this is a fixed 8 char width 0-padded integer: {08}"`

`"hex 8 char width zero padded integer: {08x}"`

`"0-padded integer to the digits that the datatype's max/min value has: {0W}"`

`"0-padded hex integer to the datatype's nibble count: {0Nx}"`

`"Escaped open brace: {{"`

Build and test
==================

Linux
------
Meson is used.

debian/ubuntu example:

`git submodule update --init --recursive`

`sudo apt install ninja-build python3-pip`
`sudo -H pip3 install meson`

`MYBUILDDIR=build`
`meson $MYBUILDDIR  --buildtype=release OR > meson`
`ninja -C $MYBUILDDIR`
`ninja -C $MYBUILDDIR test`

Windows
-------

Untested. It's almost surely broken, as the thin-wrapping C multiplatform
library I'm using hasn't been run in Windows since very long ago. I have no
Windows installs available.

Without build system (untested)
-------------------------------

Compile every file under "src" and use "include" as include dir. This will still
have challenges:

-Compiling the "base-library" dependency. It follows the same structure.
 Compile everything under "src" and include "include". Statically link with
 the "base", "nonblock" and "time extras" libraries.

-Generating "include/malc/config.h", a sample from my machine as of now:

```C
#ifndef __MALC_CONFIG_H__
#define __MALC_CONFIG_H__

/* autogenerated file, don't edit */

#define MALC_VERSION_MAJOR 1
#define MALC_VERSION_MINOR 0
#define MALC_VERSION_REV   0
#define MALC_VERSION_STR   "1.0.0"
#define MALC_BUILTIN_COMPRESSION 0
#define MALC_PTR_COMPRESSION 0

#endif /* __MALC_CONFIG_H__ */
```
