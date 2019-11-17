Description
===========

A C11/C++11 low-latency wait-free producer (when using Thread Local Storage)
asynchronous data logger with type-safe strings.

Based on the lessons learned on its older C++ counterpart mini-async-log.

Features
========

- Very high performance. Hard to be faster for a generic library. Beats
  "mini-async-log" in most configurations.

- Various memory (log entry) sources: Thread Local Storage buffer, common
  bounded buffer (configurable to have one for each CPU) and custom allocators
  (defaults to the heap).

- Type-safe format strings. On C11 achieved through C11 type-generic expressions
  and (unfortunately) brutal preprocessor abusing. On C++11 they are validated
  at compile time too.

- C++ wrapped. It can select between throwing and non-throwing wrappers so it
  can be used in project with non-throwing coding standards.

- Not a singleton. No hidden threads: the client application can run the
  logger's consumer thread main loop from an existing (maybe shared for other
  purposes) thread if desired.

- Basic security features: Log entry rate limiting and newline removal.

- Extensible log destinations (sinks).

- Compile-time removable severities.

- Lazy evaluated parameters. If the log call is filtered out because of a low
  severity it doesn't get evaluated.

- Zero-copy logging of strings/memory ranges: achieved by just invoking a
  destruction callback from where manual deallocation or reference counting can
  be done.

- Decent test coverage.

Log format strings
==================

The format strings have this (common) bracket delimited form:

> "this is value 1: {} and this is value 2: {}"

The opening brace '{' is escaped by doubling it. The close brace doesn't need
escaping.

Depending on the data type modifiers inside the brackets are accepted. Those try
to match own printf's modifiers.

As a reminder, "printf" format strings are composed like this:

%[flags]\[width][.precision]\[length]specifier

Where the valid chars for each field are (on C99):

-flags: #, 0, +, -
-width: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
-precision: ., 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, .*
-length: h, hh, l, ll, j, z, t, L
-specifiers: d, i, u , o, x, X, f, F, e, E, g, G, a, A, c, s, p, n, %

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

integrals
---------

-flags: #, 0, +, -
-width: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, W, N
-specifiers: o, x, X

floating point
--------------

-flags: #, 0, +, -
-width: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
-precision: ., 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
-specifiers: f, F, e, E, g, G, a, A

others
------

Other types, like pointers, strings, etc. accept no modifiers.

examples
--------

> "this is a fixed 8 char width 0-padded integer: {08}"

> "hex 8 char width zero padded integer: {08x}"

> "0-padded integer to the digits that the datatype's max/min value has: {0W}"

> "0-padded hex integer to the datatype's nibble count: {0Nx}"

> "Escaped open brace: {{"

Build and test
==================

Meson is used. Ubuntu:

> sudo apt install ninja-build python3-pip
> sudo -H pip3 install meson

> git submodule update --init --recursive
> meson <your_stage_dir>  --buildtype=release
> ninja -C <your_stage_dir>
> ninja -C <your_stage_dir> test

Windows:

Untested. It's almost surely broken, as the thin-wrapping C multiplatform
library I'm using hasn't run in Windows in ages, as I have no Windows installs
available.
