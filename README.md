# EpochTime

This is an Arduino utility library for converting between time in parts as used by core `gmtime` functions and epoch time as a single 32-bit or 64-bit (`time_t`) integer.

The library provides two main classes

- The epochTime class, which creates a time object aware of its own internal epoch.
- The TimeUtils class, which is a class of static functions for
  - Formatting timestamps into ISO8601 and custom string formats
  - Validating timestamp sanity
  - Abstracting processor/Arduino core epoch and timezone settings

Dealing with time is **hard**!
This library only supports the bare minimum necessary to work with logger clocks and convert between different epoch types.
It does not support full time zones (only static offsets from UTC), daylight savings time, or many other complications of time.

If you thought handling time was simple, read this: https://gist.github.com/timvisee/fcda9bbdff88d45cc9061606b4b923ca
