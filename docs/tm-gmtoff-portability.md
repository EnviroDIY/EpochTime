# `tm_gmtoff` portability and conversion policy

## EpochTime policy

When an Arduino core defines `__TM_GMTOFF`, `TimeUtils::tmToEpochTime()` saves
the corresponding `struct tm` offset in the resulting `epochTime` object. The
offset is retained only as formatting metadata.

The offset does not participate in conversion to a timestamp. Before EpochTime
passes a temporary `struct tm` to `mktime()`, it replaces the offset with a
deliberately invalid sentinel. This prevents implementations that optionally
inspect `tm_gmtoff` from treating it as a valid UTC offset or timezone-transition
hint. `mktime()` therefore interprets the standard calendar fields and
`tm_isdst` using the processor core's local timezone.

`TimeUtils::sameTime()` follows the same policy. It compares the timestamps
compiled from the standard calendar fields and `tm_isdst`, while deliberately
excluding `tm_gmtoff`. The original input structures are not modified.

If `__TM_GMTOFF` is unavailable, EpochTime stores a formatting offset of zero.

## What `__TM_GMTOFF` means

`__TM_GMTOFF` is not a general C, POSIX, GNU, BSD, or Arduino feature-test
macro. It is a Newlib/Picolibc port-configuration macro whose replacement text
is the name of the optional `struct tm` member. A port can enable the field with
a definition such as:

```cpp
#define __TM_GMTOFF tm_gmtoff
```

The relevant Newlib/Picolibc declaration is effectively:

```cpp
#ifdef __TM_GMTOFF
long __TM_GMTOFF;
#endif
```

Other C libraries can expose a member named `tm_gmtoff` without defining
`__TM_GMTOFF`. EpochTime's conditional support applies specifically to cores
that define the Newlib-style macro.

## Installed PlatformIO core audit

The local audit on October 30, 2026 examined both:

- `C:\platformio\platforms`, which contains PlatformIO platform definitions and
  selects each framework and toolchain
- `C:\platformio\packages`, which contains the framework sources, C library
  headers, and cross-compilers that define the effective Arduino environment

The `%LOCALAPPDATA%\Arduino15\packages` Arduino IDE installation was outside the
agreed scope.

The installed PlatformIO platforms were:

- Atmel AVR and megaAVR
- Atmel SAM and SAMD
- Espressif ESP32 and ESP8266
- Intel ARC32
- Microchip PIC32
- Nordic nRF52
- Raspberry Pi, using the Arduino Mbed framework
- Renesas RA
- STMicroelectronics STM32
- Teensy

The audit searched all installed platform, framework, and toolchain files for
`__TM_GMTOFF`, `tm_gmtoff`, and `__tm_gmtoff`. It also directly preprocessed
and compiled a `tm_gmtoff` member access with each usable installed
cross-compiler in GNU C++11 mode.

No installed Arduino core defines `__TM_GMTOFF`, and no installed core exposes
an unconditional `tm_gmtoff` or equivalent member. The only embedded C library
matches were conditional Newlib/Picolibc declarations guarded by
`#ifdef __TM_GMTOFF`. Consequently, EpochTime's `__TM_GMTOFF` code is currently
unreachable on all of the locally installed PlatformIO Arduino cores.

This is an installation snapshot, not a claim about every past or future
Arduino core.

## Standards and C library behavior

ISO C does not define `tm_gmtoff`. POSIX first standardized `tm_gmtoff` and
`tm_zone` in POSIX.1-2024. POSIX.1-2024 permits `mktime()` to consult
`tm_gmtoff` only as a safe hint when choosing between two valid results around
an ambiguous timezone transition. It does not define `tm_gmtoff` as an
arbitrary input timezone for `mktime()`, and implementations may ignore the
input value.

Relevant implementation behavior differs:

- GNU libc explicitly ignores the input values of `tm_gmtoff` and `tm_zone`.
  It uses the active timezone state and overwrites the derived fields in the
  normalized result.
- Newlib documents `mktime()` as interpreting its input as local time. Its
  ordinary implementation does not use the optional `__TM_GMTOFF` member as an
  arbitrary input offset.
- OpenBSD exposes `tm_gmtoff`, but its current `mktime()` implementation does
  not read the input field. It fills the field in the normalized result.
- Modern NetBSD and IANA tzcode can use a plausible input `tm_gmtoff` as a
  checked heuristic for an ambiguous local time. They do not use it as a
  general fixed-offset conversion request.
- Modern FreeBSD imports the same IANA tzcode family and has the same limited
  heuristic behavior.

Because these behaviors are inconsistent, EpochTime does not ask `mktime()` to
apply `tm_gmtoff`. Code that needs an offset to determine the represented
instant should construct an `epochTime` from a timestamp and explicit offset,
or explicitly convert the offset with the EpochTime conversion functions.

## References

- [IANA time zone theory and POSIX.1-2024 notes](https://www.iana.org/time-zones/theory)
- [Austin Group Issue 1797 wording for `mktime()` and `tm_gmtoff`](https://www.mail-archive.com/austin-group-l%40opengroup.org/msg13058.html)
- [GNU libc broken-down time documentation](https://snapshots.sourceware.org/glibc/trunk/2026-04-15_23-51_1776297061/manual/html_node/Broken_002ddown-Time.html)
- [Newlib date and time documentation](https://www.sourceware.org/newlib/libc.html)
- [OpenBSD `struct tm` declaration](https://raw.githubusercontent.com/openbsd/src/master/include/time.h)
- [OpenBSD `mktime()` implementation](https://raw.githubusercontent.com/openbsd/src/master/lib/libc/time/localtime.c)
- [NetBSD `mktime()` implementation](https://raw.githubusercontent.com/NetBSD/src/trunk/lib/libc/time/localtime.c)
- [FreeBSD tzcode updates](https://cgit.freebsd.org/src/log/contrib/tzcode)


<!-- cSpell:ignore gmtoff mktime isdst newlib LOCALAPPDATA picolibc libc  -->
