Description
===========

A C11 low-latency wait-free producer (when using Thread Local Storage)
asynchronous data logger with type-safe strings.

Based on the lessons learned on its older C++ counterpart mini-async-log.

Features
========

- Very high performance. Hard to be faster for a generic library. Beats
  "mini-async-log" in most configurations.

- Various memory (log entry) sources: Thread Local Storage buffer, common
  bounded buffer (configurable to have one for each CPU) and custom allocators
  (defaults to the heap).

- Type-safe format strings. Achieved through C11 type-generic expressions and
  (unfortunately) brutal preprocessor abusing.

- C++ compatible/compilable.

- The client application can run the logger's consumer thread main loop from an
  existing (maybe shared for other purposes) thread if desired.

- Basic security features: Log entry rate limiting and newline removal.

- Extensible log destinations (sinks).

- Compile-time removable severities.

- Zero-copy logging of strings/memory ranges: achieved by just invoking a
  destruction callback from where manual deallocation or reference counting can
  be done.

- Decent test coverage.

Log format strings
==================

The format strings have this form:

> "this is value 1: {} and this is value 2: {}"

These are like printf strings where the length related specifiers are detected
automatically. So you never need to specify e.g hh, llu, d, etc.

All the strings inside the braces will be passed internally to printf as format
modifiers, e.g.

> "this is a fixed 8 char width zero padded integer: {08}"

You can use all the other non-length specifiers:

> "hex 8 char width zero padded integer: {08x}"

The new "W" specifier is replaced by the maximum character count that an
integer can allocate (3 for 1 byte, 5 for 2 bytes, 10 for 4 bytes and 20 for 8
bytes):

> "full width zero padded integer: {0W}"

The new "N" specifier is replaced by the number of nibbles of an hexadecimal
integer:

> "full width hex zero padded integer: {0Nx}"

The "*" width and ".*" precision specifiers are unsupported.

The opening brace '{' is escaped by doubling it. The close brace doesn't need
escaping:

> "Escaped open brace: {{"

Build and test
==================

Meson 0.41 is used. Ubuntu:

> sudo apt install ninja-build python3-pip
> sudo -H pip3 install meson

> git submodule update --init --recursive
> meson <your_stage_dir>  --buildtype=release
> ninja -C <your_stage_dir>
> ninja -C <your_stage_dir> test

Windows:

Untested. It's almost surely broken. If it's broken on the preprocessor code
(Visual Studio preprocessor quirks) then the preprocessor hackery can be
replaced with template hackery.

TODO
==================

-Investigate faster, platform-specific options for timestamping.

-Implement in-place deallocation for failed entries with dynamic fields if
sane/possible. No use case seems to be broken: refcount, no dealloc and
malloc/free. Change the signature, so the called desctructor can know if it's an
in-place deallocation. As this is new behavior it should be explicitly enabled.
