Current status
==============
Proof of concept of the upper asynchronous printf structure which allows to
defer the string evaluation (using C11 _Generic + preprocessor hackery).

Description
===========


Features (Planned)
==================

TODO

- Security: Sanitized log strings (removing newlines: attacker trying to fake
    log entries).
- Security: Rate limit of the same log entry (attacker trying to erase
    information of the logs by forcing rotation).