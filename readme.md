![](https://github.com/RafaGago/mini-async-log-c/workflows/ci_linux/badge.svg)
![](https://github.com/RafaGago/mini-async-log-c/workflows/ci_windows/badge.svg)

Description
===========

A C11/C++11 low-latency wait-free producer (when using Thread Local Storage)
asynchronous textual data logger with type-safe strings.

Based on the lessons learned on its older C++-only counterpart "mini-async-log".

AFAIK, this may be the fastest generic data logger for C or C++.

Features
========

Common:

- Dual C/C++ library. The main implementation is C11.

- wait-free fast-path (when using thread-local storage).

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

- Program runtime not affected by clock changes. Doesn't use any non-monotonic
  blocking mechanism.

- Log files not affected by clock changes. The log files have a monotonic clock.
  Conversion to calendar time is possible by using a timestamp embedded on the
  file name.

- Decent test coverage.

C++ only:

- The instance control functions can be selected to return error codes, matching
  more or less the C interface, or to throw exceptions.

  This logger can be used in projects with non-throwing coding standards.
  Projects that are willing to use exceptions can write less error handling
  boilerplate by enabling the exception based interface. The logging functions
  are error-code based only (noexcept), as they are free functions on the
  fast-path.

- Can log std::ostream-able types by value or wrapped in a smart pointer.

- Can log std::string, smart pointers to std::string and smart pointers to
  std::vector containing arithmetic types.

- It allows adding custom logging for additional data types.

- It almost doesn't leak any C function or data type to the global namespace,
  and the few they do are prefixed with "malc_".

Design
======

The life of a log message under this logger is.

At compile time
---------------

- The format string is validated (C++ only).

- A "static const" data structure is created containing the format string, the
  severity, a code for each data type and the number of compressed datatypes (if
  enabled). All the previous data can be passed with just the overhead of one
  pointer.

At run time
---------------

- A log call is made.

- The passed arguments are stored on the stack after being passed through a
  type-specific conversion function, which leaves the data type ready to
  serialize and to retrieve it size on the wire. The conversion function is most
  cases just doing nothing. Easy for the compiler to optimize away.

- The required payload size for serializing the unformatted log entry is
  computed, taking as input the transformed type on the step above. For simple
  cases it's just a compile time constant (sizeof (type)). Again easy to be
  optimized away.

- Memory for an intrusive linked list node and all the serialization payload is
  requested from the logger in a single contiguous memory chunk.

- The pointer to the "static const" data structure and the payload data is
  serialized on the memory chunk after the intrusive linked list node (unused at
  this point).

- The intrusive node is passed to the logging queue.

- At some point the consumer thread fetches the node, deserializes it, applies
  formatting, forwards it to the log destinations/sinks and then frees the node
  memory.

The log queue
-------------

It's the well known Dmitry Vyukov's intrusive MPSC queue. Wait-free producers.
Blocking consumer. Perfect for this use case. Very good performance under low
contention and good performance when contended.

The memory
----------

3 configurable memory sources:

- Thread Local Storage. A per-thread bounded SPSC array-based cache-friendly
  memory queue that has to be explictly iniatized by each thread using it.

- Global bounded size queue. Again a Dmitry Vyukov queue. This time it's MPMC
  one but broken in prepare-commit blocks and with a custom modification to
  accept variable-size allocations (at the expense of fairness unfortunately).
  Can be configured to use one queue per CPU to try to fight contention. This
  queue has extremely good performance on the uncontended case but doesn't scale
  as good as the modern Linux heap allocator under big contention from many
  threads. It's included because bounded queues are desirable sometimes.

- Allocator. Defaults to the heap but can allocate from any user-provided
  source.

All three sources can be used together, separately or mixed. The priority order
is: TLS, bounded, allocator.

Formatting
----------

This is already hinted, there is no formatting on the producer-side. It happens
on the consumer side and its cost is masked by file-io.

Usage Quickstart
==================

Look at the examples folder.

https://github.com/RafaGago/mini-async-log-c/tree/master/example/src

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

- flags: `#, 0, +, -`

- width: `0, 1, 2, 3, 4, 5, 6, 7, 8, 9`

- precision: `., 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, .*`

- length: `h, hh, l, ll, j, z, t, L`

- specifiers: `d, i, u , o, x, X, f, F, e, E, g, G, a, A, c, s, p, n, %`


Malc autodetecs the datatypes passed to the log strings, so the "length"
modifiers and signedness "specifiers" are never required. Which specifiers can
be used on which data type is also restricted only to those that make sense.

Malc adds a non-numeric precision specifier: 'w'

'w' represents the maximum digit count excluding sign that the maximum value of
a given type can have. It is aware if the type is going to be printed in
decimal, hexadecimal or octal base.

On malc there are two precision modifier groups [0-9], w. Both of them are
mutually exclusive. Only one of them can be specified, the result of not
doing so is undefined.

Next comes a summary of the valid modifiers for each type.

integrals (or std::vector of integrals)
---------------------------------------

- flags: `#, 0, +, -`

- width: `0, 1, 2, 3, 4, 5, 6, 7, 8, 9`

- precision: `., 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, w`

- specifiers: `o, x, X`

floating point (or std::vector of floating point)
-------------------------------------------------

- flags: `#, 0, +, -`

- width: `0, 1, 2, 3, 4, 5, 6, 7, 8, 9`

- precision: `., 0, 1, 2, 3, 4, 5, 6, 7, 8, 9`

- specifiers: `f, F, e, E, g, G, a, A`

others
------

Other types, like pointers, strings, etc. accept no modifiers.

examples
--------

`"this is a fixed 8 char width 0-padded integer: {08}"`

`"hex 8 char width zero padded integer: {08x}"`

`"0-padded integer to the digits that the datatype's max/min value has: {.w}"`

`"0-padded hex integer to the datatype's nibble count: {.wx}"`

`"Escaped open brace: {{"`

Build and test
==============

Linux
------
Meson is used.

debian/ubuntu example:

```sh
git submodule update --init --recursive

sudo apt install ninja-build python3-pip
sudo -H pip3 install meson

MYBUILDDIR=build
meson $MYBUILDDIR  --buildtype=release
ninja -C $MYBUILDDIR
ninja -C $MYBUILDDIR test
```

Windows
-------

Acquire meson and ninja, if you are already using Python on Windows you may want
to intall meson by using a python package manager (e.g. pip) and then install
Ninja (ninja-build) separately.

If you don't, the easiest way to add all the dependecies is to download meson +
Ninja as an MSI installer from meson's site:

https://mesonbuild.com/Getting-meson.html

Once you have meson installed the same steps as on Linux apply. Notice that:

* meson can generate Visual Studio solutions.
* If you use meson with Ninja and the Microsoft compiler you need to export the
  Visual Studio environment before. This is done by running "vcvarsall.bat" or
  "vcvars32.bat/vcvars64.bat" depending on the Visual Studio version you have
  installed. In my machine with 2019 Community it lies in:

  "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

Without build system (untested)
-------------------------------

Compile every file under "src" and use "include" as include dir. This will still
have challenges:

- Compiling the "base-library" dependency. It follows the same structure.
  Compile everything under "src" and include "include". Statically link with
  the "base", "nonblock" and "time extras" libraries.

- Generating "include/malc/config.h", a sample from my machine as of now:

```C
#ifndef __MALC_CONFIG_H__
#define __MALC_CONFIG_H__

/* autogenerated file, don't edit */

#define MALC_VERSION_MAJOR 1
#define MALC_VERSION_MINOR 0
#define MALC_VERSION_REV   0
#define MALC_VERSION_STR   "1.0.0"
#define MALC_BUILTIN_COMPRESSION 0
#define MALC_PTR_MSB_BYTES_CUT_COUNT 0

#endif /* __MALC_CONFIG_H__ */
```

Linking
=======

This library links to a tiny own utility library that I use for C resources that
aren't project specific, so I can reuse them.

Both "malc" and this library are meant to be statically linked to your project.

If you run

```sh

DESTDIR=$PWD/dummy-install ninja -C $MYBUILDDIR install

```

You may see these files on the library section:

```
└── lib
    └── x86_64-linux-gnu
        ├── libbl-base.a
        ├── libbl-getopt.a
        ├── libbl-nonblock.a
        ├── libbl-serial.a
        ├── libbl-taskqueue.a
        ├── libbl-time-extras.a
        ├── libcmocka.a
        ├── libmalc.a
        ├── libmalcpp.a
        └── pkgconfig
            ├── bl-base.pc
            ├── bl-getopt.pc
            ├── bl-nonblock.pc
            ├── bl-serial.pc
            ├── bl-taskqueue.pc
            ├── bl-time-extras.pc
            ├── malc.pc
            └── malcpp.pc
```

Your C project will need to link against: "libbl-time-extras.a", "libbl-base.a"
"libbl-nonblock.a" and "libmalc.a". Your C++ project will need also to link
"libmalcpp.a".

Some useful build parameters and compile time macros
====================================================

Build parameters (meson)
------------------------

See "meson_options.txt". These parameters end up on "include/malc/config.h".

Compile time macros
-------------------

- "malc_fileline"

  Prepends the file and the line to the log entry literal.
  ```C
  log_error (malc_fileline "Something happended").
  ```
- MALC_STRIP_LOG_\[DEBUG|TRACE|NOTICE|WARNING|ERROR|CRITICAL\]

  Removes all the log entries with a serverity less or equal that the one
  specified.

- MALC_NO_SHORT_LOG_MACROS

  Malc defines both logging macros called e.g. "malc_error" and "log_error", when
  "MALC_NO_SHORT_LOG_MACROS" is defined "log_" prefixed macros aren't defined.

- MALC_CUSTOM_LOGGER_INSTANCE_EXPRESSION

  On the macros that doesn't take a malc instance explictly (those that don't
  have the "_i" suffix) malc invokes an macro containing an expression to get the
  logger instance. By default this macro contains "get_malc_instance()", so it
  retrieves the logger instance by a call to a function called
  "get_malc_instance()" that has to be provided by the user.

  To use an expression other than "get_malc_instance()" define the
  MALC_CUSTOM_LOGGER_INSTANCE_EXPRESSION.

  Notice that on C this expression has to return a pointer, on C++ it has to
  return a reference.

Gotchas
=======

Crash handling
--------------

This library doesn't install any crash handlers because in my opionion it is out
of the scope of a logging library.

If you want to terminate and flush the logger from a signal/exception handler,
you can call "malc_terminate (ptr, false)" from there.

Lazy-evaluation of parameters
-----------------------------

Both the C and the C++ log functions doesn't have function-like semantics:

- The passed parameters are only evaluated if the log entry is going to be
  logged. Log entries can be filtered out by severity or in case of the
  "log_|severity|_if" macro family if the conditional parameter is evaluated to
  be "false".

- As with the C "assert" macro, log entries can be stripped at compile time by
  defining the "MALC_STRIP_LOG_|SEVERITY|" macro family.

This is deliberate, so you can place expensive function calls as log arguments.

Asynchronous logging
--------------------

When timestamping at the producer thread is enabled (the default is platform
dependant), there is the theoretical possibility that some entries show
timestamps that go backwards in time some fractions of a second. This is
expected. Consider this case:

- Thread 1: gets timestamp.

- Thread 1: gets preempted by the OS scheduler.

- Thread 2: gets timestamp.

- Thread 2: posts the log entry into the queue.

- Thread 1: is schedulead again

- Thread 1: posts the log entry into the queue.

A big mutex on the queue wouldn't theoretically show this behavior, but then:

- The timestamp would get more jitter, as it will account the time waiting for
  the mutex.

- This logger wouldn't perform as it does.

Thread safety of values passed by reference to the logger
---------------------------------------------------------

All values passed by pointer to malc are assumed to never be modified again by
any other thread. The results of doing so are undefined.

Tradeoffs
=========

When using this library remember that this is an asynchronous logger designed to
optimize the caller/producer site. Consider if your application really needs an
asynchronous logger and if performance at the call site is more important than
the added complexity of having an asynchronous logger.

Tradeoffs:

- It trades code size at the call site for performance. On C each log macro
  call puts at least one branch and two function calls with error code checks at
  the calling site. This is without counting parameters. For each parameter
  there is at least a (hopefully optimized away) call to "memcpy" too.

  On C++ it does exactly the same operations, but considering the language
  standards I wouldn't call it more bloated than any other implementation using
  variadic templates for logging the calls.

- Some C++ features may be harder to implement because the core is C. Time will
  tell.
