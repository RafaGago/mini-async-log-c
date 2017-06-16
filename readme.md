Current status
==============
Proof of concept of the upper asynchronous printf structure which allows to
defer the string evaluation (using C11 _Generic + preprocessor hackery).

Description
===========

Format string
=============

The format strings are have this form:

> "this is value 1: {} and this is value 2: {}"

The length related specifiers are autodected. So you never need to specify e.g
hh, llu, d, etc.

All the strings inside the braces will be passed to print as format, e.g.

> "this is a fixed 8 char width zero padded integer: {08}"

You can use all the other non-length specifiers:

> "hex 8 char width zero padded integer: {08x}"

The new "W" specifier is replaced by the maximum character count that an
integer can allocate (3 for 1 byte, 5 for 2 bytes, 10 for 4 bytes and 20 for 8 bytes):

> "full width zero padded integer: {0W}"

The new "N" specifier is replaced by the number of nibbles of an integer:

> "full width hex zero padded integer: {0Nx}"

The "*" width and ".*" precision specifiers are unsupported.

The opening brace '{' and '}' is escaped by a doubling it. The close brace never
needs escaping:

> "Escaped open brace: {{"

Features (Planned)
==================

TODO

- Enqueue strategy: Heap, thread local storage and bounded.
- Security: Sanitized log strings (removing newlines: attacker trying to fake
    log entries).
- Security: Rate limit of the same log entry (attacker trying to erase
    information of the logs by forcing rotation).

Build
==================

Meson 0.41 is used. In Ubuntu:

> sudo -H pip3 install meson