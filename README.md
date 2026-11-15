# EpochTime

This is an Arduino utility library for converting between time in parts as used by core `gmtime` functions and epoch time as a single 32-bit or 64-bit (`time_t`) integer.

The library provides two main classes

- The epochTime class, which creates a time object aware of its own internal epoch.
- The TimeUtils class, which is a class of static functions for
  - Formatting timestamps into ISO8601 and custom string formats
  - Formatting deterministic compact and calendar timestamps into caller-owned buffers
  - Formatting UTC offsets as `Z` or signed hours and minutes
  - Validating timestamp sanity
  - Abstracting processor/Arduino core epoch and timezone settings

Dealing with time is **hard**!
This library only supports the bare minimum necessary to work with logger clocks and convert between different epoch types.
It does not support full time zones (only static offsets from UTC), daylight savings time, or many other complications of time.

## UTC offset units

All UTC-offset parameters use signed seconds, with positive values east of UTC and negative values west of UTC.
Values from -24 through +24 are interpreted as seconds, not hours.
Code using legacy hour-valued arguments must convert them to seconds, such as replacing `-5` with `-5 * 3600L` for UTC-5.
ISO 8601 offsets must be a whole number of minutes, so fractional-hour offsets such as UTC+05:30 are supported.

The always-available buffer APIs do not depend on Arduino `String` or `strftime()`.
`TimeUtils::formatTimestamp()` preserves the input `epochTime` object's retained offset and supports `YYYYMMDD`, `YYYYMMDD_HHMMSS`, and `YYYY-MM-DD HH:MM:SS` output.
`TimeUtils::formatUtcOffset()` produces `Z`, `+HH:MM`, or `-HH:MM` output.

Parts of this library's functionality exits in **many** Arduino libraries supporting clocks, but I found none that had complete enough or well enough abstracted functionality to be portable, so I created this one.

If you thought handling time was simple, read this: https://gist.github.com/timvisee/fcda9bbdff88d45cc9061606b4b923ca
