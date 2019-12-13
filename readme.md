Description
===========

A C11/C++11 low-latency wait-free producer (when using Thread Local Storage)
asynchronous textual data logger with type-safe strings.

Based on the lessons learned on its older C++-only counterpart "mini-async-log".

Design
======

The life of a log message under this logger is.

At compile time
---------------

-The format string is validated (C++ only).

-A "static const" data structure is created containing the format string, the
 severity, a code for each data type and the number of compressed datatypes (if
 enabled). All the previous data can be passed with just the overhead of one
 pointer.

At run time
---------------

-A log call is made.

-The passed arguments are stored on the stack after being passed through a
 type-specific conversion function, which leaves the data type ready to
 serialize and to retrieve it size on the wire. The conversion function is most
 cases just doing nothing. Easy for the compiler to optimize away.

-The required payload size for serializing the unformatted log entry is
 computed, taking as input the transformed type on the step above. For simple
 cases it's just a compile time constant (sizeof (type)). Again easy to be
 optimized away.

-Memory for an intrusive linked list node and all the serialization payload is
 requested from the logger in a single contiguous memory chunk.

-The pointer to the "static const" data structure and the payload data is
 serialized on the memory chunk after the intrusive linked list node (unused at
 this point).

-The intrusive node is passed to the logging queue.

-At some point the consumer thread fetches the node, deserializes it, applies
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

-Thread Local Storage. A per-thread bounded SPSC array-based cache-friendly
 memory queue that has to be explictly iniatized by each thread using it.

-Global bounded size queue. Again a Dmitry Vyukov queue. This time it's MPMC one
 but broken in prepare-commit blocks and with a custom modification to accept
 variable-size allocations (at the expense of fairness unfortunately). Can be
 configured to use one queue per CPU to try to fight contention. This queue has
 extremely good performance on the uncontended case but doesn't scale as good as
 the modern Linux heap allocator under big contention. It's included because
 bounded queues are desirable sometimes.

-Allocator. Defaults to the heap but can allocate from any user-provided source.

All three sources can be used together, separately or mixed. The priority order
is: TLS, bounded, allocator.

Formatting
----------

This is already hinted, there is no formatting on the producer-side. It happens
on the consumer side and it's cost is masked by file-io.

Features
========

Common:

- Dual C/C++ library. The main implementation is C11.

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
  are error-code based only (noexcept), as they are free functions on the
  fast-path.

- Can log std::ostream-able types by value or wrapped in a shared pointer.

- Can log std::string, smart pointers to std::string and smart pointers to
  std::vector containing arithmetic types.

- It allows adding custom logging for additional data types.

- It almost doesn't leak any C function or data type to the global namespace,
  and the few they do are prefixed.

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
==============

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
Some useful build parameters and compile time macros
====================================================

Build parameters (meson)
------------------------

See "meson_options.txt". These parameters end up on "include/malc/config.h".

Compile time macros
-------------------

-"malc_fileline"

 Prepends the file and the line to the log entry literal.
 ```C
 log_error (malc_fileline "Something happended").
 `--
-MALC_STRIP_LOG_\[DEBUG|TRACE|NOTICE|WARNING|ERROR|CRITICAL\]

 Removes all the log entries with a serverity less or equal that the one 
 specified.

-MALC_NO_SHORT_LOG_MACROS

 Malc defines both logging macros called e.g. "malc_error" and "log_error", when
 "MALC_NO_SHORT_LOG_MACROS" is defined "log_" prefixed macros aren't defined.

-MALC_GET_LOGGER_INSTANCE_FUNCNAME
 
 On the macros that doesn't take a malc instance explictly (those that don't 
 have the "_i" suffix) malc makes a call to an expression to be defined by the 
 user called "get_malc_logger_instance()" to provide the malc instance (this 
 library is not a singleton). The MALC_GET_LOGGER_INSTANCE_FUNCNAME allows to 
 change the name of the call done to retrieve the instance.

Gotchas
=======

Lazy-evaluation of parameters
-----------------------------

Both the C and the C++ log functions doesn't have function-like semantics:

-The passed parameters are only evaluated if the log entry is going to be 
 logged. Log entries can be filtered out by severity or in case of the 
 "log_|severity|_if" macro family if the conditional parameter is evaluated to 
 be "false".

-As with the C "assert" macro, log entries can be stripped at compile time by
 defining the "MALC_STRIP_LOG_|SEVERITY|" macro family.

This is deliberate, so you can place expensive function calls as log arguments.

Asynchronous logging
--------------------

When timestamping at the producer thread is enabled, there is the theoretical 
possibility that some entries show timestamps that go backwards in time some
fractions of a second. This is expected. Consider this case:

-Thread 1: gets timestamp.
-Thread 1: gets preempted by the OS scheduler.
-Thread 2: gets timestamp.
-Thread 2: posts the log entry into the queue.
-Thread 1: is schedulead again 
-Thread 1: posts the log entry into the queue.

A big mutex on the queue wouldn't theoretically show this behavior, but then:

-The timestamp would get more jitter, as it will account the time waiting for 
 the mutex.
-This logger wouldn't perform as it does.

Thread safety of values passed by reference to the logger
---------------------------------------------------------

All values passed by pointer to malc are assumed to never be modified again by
any other thread. The results of doing so are undefined.
