/** ============================================================================
 * @example{lineno} FormattingTests.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Runs edge-case tests for the epochTime and TimeUtils conversion,
 * comparison, formatting, and sanity-checking functions on an Arduino.
 * ========================================================================== */

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

#include <EpochTime.h>

#if (defined(ARDUINO_NRF52840_FEATHER)) && !defined(ADAFRUIT_TINYUSB_H_)
#include <Adafruit_TinyUSB.h>  // for Serial
#endif

uint16_t checks   = 0;
uint16_t failures = 0;
uint16_t skips    = 0;

void printInt64(int64_t value) {
    uint64_t magnitude;
    if (value < 0) {
        Serial.print('-');
        magnitude = static_cast<uint64_t>(-(value + 1));
        ++magnitude;
    } else {
        magnitude = static_cast<uint64_t>(value);
    }

    char    digits[21];
    uint8_t length = 0;
    do {
        digits[length++] = static_cast<char>('0' + magnitude % 10);
        magnitude /= 10;
    } while (magnitude > 0);
    while (length > 0) { Serial.print(digits[--length]); }
}

void checkEqual(const __FlashStringHelper* name, int64_t expected,
                int64_t actual, int line) {
    ++checks;
    if (expected == actual) {
        Serial.print(F("PASS: "));
        Serial.println(name);
        return;
    }

    ++failures;
    Serial.print(F("FAIL line "));
    Serial.print(line);
    Serial.print(F(": "));
    Serial.print(name);
    Serial.print(F("; expected "));
    printInt64(expected);
    Serial.print(F(", got "));
    printInt64(actual);
    Serial.println();
}

void checkString(const __FlashStringHelper* name, const char* expected,
                 const char* actual, int line) {
    ++checks;
    if (strcmp(expected, actual) == 0) {
        Serial.print(F("PASS: "));
        Serial.println(name);
        return;
    }

    ++failures;
    Serial.print(F("FAIL line "));
    Serial.print(line);
    Serial.print(F(": "));
    Serial.print(name);
    Serial.print(F("; expected \""));
    Serial.print(expected);
    Serial.print(F("\", got \""));
    Serial.print(actual);
    Serial.println(F("\""));
}

void skip(const __FlashStringHelper* name, const __FlashStringHelper* reason) {
    ++skips;
    Serial.print(F("SKIP: "));
    Serial.print(name);
    Serial.print(F("; "));
    Serial.println(reason);
}

#define CHECK_EQUAL(name, expected, actual)             \
    checkEqual(F(name), static_cast<int64_t>(expected), \
               static_cast<int64_t>(actual), __LINE__)
#define CHECK_STRING(name, expected, actual) \
    checkString(F(name), expected, actual, __LINE__)

void printSection(const __FlashStringHelper* name) {
    Serial.println();
    Serial.println(name);
    Serial.println(F("----------------------------------------"));
}

void printEpochName(epochStart epoch) {
    switch (epoch) {
        case epochStart::unix_epoch: Serial.print(F("Unix")); break;
        case epochStart::y2k_epoch: Serial.print(F("Y2K")); break;
        case epochStart::gps_epoch: Serial.print(F("GPS")); break;
        case epochStart::nist_epoch: Serial.print(F("NIST")); break;
        default: Serial.print(F("unknown")); break;
    }
}

tm makeTimeParts(int year, int month, int day, int hour, int minute, int second,
                 int32_t utcOffsetSeconds) {
    tm parts       = {};
    parts.tm_year  = year - 1900;
    parts.tm_mon   = month - 1;
    parts.tm_mday  = day;
    parts.tm_hour  = hour;
    parts.tm_min   = minute;
    parts.tm_sec   = second;
    parts.tm_isdst = 0;
#ifdef __TM_GMTOFF
    parts.__TM_GMTOFF = utcOffsetSeconds;
#else
    (void)utcOffsetSeconds;
#endif
    return parts;
}

void testOffsetAndEpochConversions() {
    printSection(F("Offset and epoch conversions"));
    const timestamp_t unixUtc   = static_cast<timestamp_t>(1782864000LL);
    const int32_t     estOffset = -5 * 3600L;
    const timestamp_t local     = unixUtc + estOffset;

    epochTime localTime(local, estOffset, epochStart::unix_epoch);
    CHECK_EQUAL("negative offset normalizes to UTC", unixUtc,
                localTime.getTimestamp());
    CHECK_EQUAL("negative offset round trips", local,
                localTime.getTimestamp(estOffset));

    const int32_t indiaOffset = 5 * 3600L + 30 * 60L;
    epochTime     indiaTime(unixUtc + indiaOffset, indiaOffset,
                            epochStart::unix_epoch);
    CHECK_EQUAL("positive half-hour offset normalizes to UTC", unixUtc,
                indiaTime.getTimestamp());
    CHECK_EQUAL("positive half-hour offset round trips", unixUtc + indiaOffset,
                indiaTime.getTimestamp(indiaOffset));
    CHECK_EQUAL("equal instants compare equal", 1, localTime == indiaTime);
    CHECK_EQUAL("different instants compare unequal", 1,
                localTime != epochTime(unixUtc + 1));

    const timestamp_t y2k = unixUtc - EPOCH_UNIX_TO_Y2K;
    CHECK_EQUAL("Unix to Y2K", y2k,
                TimeUtils::convertEpoch(unixUtc, epochStart::unix_epoch,
                                        epochStart::y2k_epoch));
    CHECK_EQUAL("Y2K round trip", unixUtc,
                TimeUtils::convertEpoch(y2k, epochStart::y2k_epoch,
                                        epochStart::unix_epoch));

    const timestamp_t nist = unixUtc + EPOCH_NIST_TO_UNIX;
    CHECK_EQUAL("Unix to NIST", nist,
                TimeUtils::convertEpoch(unixUtc, epochStart::unix_epoch,
                                        epochStart::nist_epoch));
    CHECK_EQUAL("NIST round trip", unixUtc,
                TimeUtils::convertEpoch(nist, epochStart::nist_epoch,
                                        epochStart::unix_epoch));

    const timestamp_t gps = unixUtc - EPOCH_UNIX_TO_GPS + 18;
    CHECK_EQUAL("Unix to GPS includes leap seconds", gps,
                TimeUtils::convertEpoch(unixUtc, epochStart::unix_epoch,
                                        epochStart::gps_epoch));
    CHECK_EQUAL("GPS round trip", unixUtc,
                TimeUtils::convertEpoch(gps, epochStart::gps_epoch,
                                        epochStart::unix_epoch));
    CHECK_EQUAL("GPS offset is normalized before leap conversion", 362793600LL,
                TimeUtils::convertOffsetAndEpoch(
                    static_cast<timestamp_t>(46810801LL), estOffset,
                    epochStart::gps_epoch, 0, epochStart::unix_epoch));
}

void testFormattingAndSanity() {
    printSection(F("Formatting and sanity boundaries"));
    const timestamp_t unixUtc   = static_cast<timestamp_t>(1782864000LL);
    const int32_t     estOffset = -5 * 3600L;
    const timestamp_t local     = unixUtc + estOffset;
    epochTime         localTime(local, estOffset, epochStart::unix_epoch);

    char buffer[40] = {};
    TimeUtils::formatISO8601(buffer, local, -5, epochStart::unix_epoch);
    CHECK_STRING("raw ISO timestamp retains UTC-5 fields",
                 "2026-06-30T19:00:00-05:00", buffer);
    TimeUtils::formatISO8601(buffer, localTime, 14);
    CHECK_STRING("ISO conversion crosses into UTC+14",
                 "2026-07-01T14:00:00+14:00", buffer);
    TimeUtils::formatISO8601(buffer, localTime, -12);
    CHECK_STRING("ISO conversion crosses into UTC-12",
                 "2026-06-30T12:00:00-12:00", buffer);

    TimeUtils::formatISO8601(buffer, static_cast<timestamp_t>(1709251199LL), 0,
                             epochStart::unix_epoch);
    CHECK_STRING("leap-day final second formats correctly",
                 "2024-02-29T23:59:59+00:00", buffer);
    TimeUtils::formatISO8601(buffer, static_cast<timestamp_t>(1767225599LL), 0,
                             epochStart::unix_epoch);
    CHECK_STRING("year-boundary final second formats correctly",
                 "2025-12-31T23:59:59+00:00", buffer);

#if EPOCHTIME_ENABLE_STRFTIME
    CHECK_EQUAL("custom formatter reports its length", 19,
                TimeUtils::formatDateTime(buffer, sizeof(buffer),
                                          "%Y-%m-%d %H:%M:%S", localTime));
    CHECK_STRING("custom formatter retains a negative offset",
                 "2026-06-30 19:00:00", buffer);

    const int32_t indiaOffset = 5 * 3600L + 30 * 60L;
    epochTime     indiaTime(unixUtc + indiaOffset, indiaOffset,
                            epochStart::unix_epoch);
    TimeUtils::formatDateTime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S",
                              indiaTime);
    CHECK_STRING("custom formatter retains a half-hour offset",
                 "2026-07-01 05:30:00", buffer);

    CHECK_EQUAL("raw custom formatter reports its length", 19,
                TimeUtils::formatDateTime(buffer, sizeof(buffer),
                                          "%Y-%m-%d %H:%M:%S", local,
                                          epochStart::unix_epoch));
    CHECK_STRING("raw custom formatter retains calendar fields",
                 "2026-06-30 19:00:00", buffer);

    char smallBuffer[8] = {'x', 'x', 'x', 'x', 'x', 'x', 'x', '\0'};
    CHECK_EQUAL("small format buffer reports failure", 0,
                TimeUtils::formatDateTime(smallBuffer, sizeof(smallBuffer),
                                          "%Y-%m-%d", localTime));
    CHECK_EQUAL("small format buffer is cleared", 0, smallBuffer[0]);
#else
    skip(F("custom date/time formatting"),
         F("EPOCHTIME_ENABLE_STRFTIME is disabled"));
#endif

    const timestamp_t earliestLocal =
        static_cast<timestamp_t>(EARLIEST_SANE_UNIX_TIMESTAMP) + estOffset;
    CHECK_EQUAL(
        "earliest sane instant is included", 1,
        TimeUtils::isTimeSane(earliestLocal, -5, epochStart::unix_epoch));
    CHECK_EQUAL(
        "second before sane range is rejected", 0,
        TimeUtils::isTimeSane(earliestLocal - 1, -5, epochStart::unix_epoch));
    CHECK_EQUAL("latest sane instant is included", 1,
                TimeUtils::isTimeSane(
                    static_cast<timestamp_t>(LATEST_SANE_UNIX_TIMESTAMP), 0,
                    epochStart::unix_epoch));
    CHECK_EQUAL("second after sane range is rejected", 0,
                TimeUtils::isTimeSane(
                    static_cast<timestamp_t>(LATEST_SANE_UNIX_TIMESTAMP) + 1, 0,
                    epochStart::unix_epoch));
}

void testCoreTimeAndTmConversions() {
    printSection(F("Core time_t and tm conversions"));
    const timestamp_t unixUtc    = static_cast<timestamp_t>(1782864000LL);
    const int32_t     estOffset  = -5 * 3600L;
    const timestamp_t local      = unixUtc + estOffset;
    const epochStart  coreEpoch  = TimeUtils::getCoreEpochStart();
    const int32_t     coreOffset = TimeUtils::getCoreTimeZone();
    const timestamp_t expectedCore =
        TimeUtils::convertEpoch(unixUtc, epochStart::unix_epoch, coreEpoch);

    CHECK_EQUAL("epochTime converts to the core epoch", expectedCore,
                TimeUtils::getTimeT(epochTime(unixUtc)));
    CHECK_EQUAL("local timestamp converts to the core epoch", expectedCore,
                TimeUtils::getTimeT(local, estOffset, epochStart::unix_epoch));

    tm                parts            = makeTimeParts(2026, 7, 1, 0, 0, 0, 0);
    const timestamp_t expectedCompiled = unixUtc - coreOffset;
    epochTime         compiled         = TimeUtils::tmToEpochTime(parts);
    CHECK_EQUAL("tm fields compile to the expected UTC instant",
                expectedCompiled, compiled.getTimestamp());

#ifdef __TM_GMTOFF
    tm        offsetParts    = makeTimeParts(2026, 6, 30, 19, 0, 0, estOffset);
    epochTime offsetCompiled = TimeUtils::tmToEpochTime(offsetParts);
    const timestamp_t expectedOffsetCompiled = unixUtc + estOffset - coreOffset;
    CHECK_EQUAL("tm offset does not adjust the compiled instant",
                expectedOffsetCompiled, offsetCompiled.getTimestamp());

    tm sameFieldsUtcOffset          = offsetParts;
    sameFieldsUtcOffset.__TM_GMTOFF = 0;
    CHECK_EQUAL("different tm offsets compile to the same instant",
                offsetCompiled.getTimestamp(),
                TimeUtils::tmToEpochTime(sameFieldsUtcOffset).getTimestamp());
#endif

#if EPOCHTIME_ENABLE_STRFTIME
#ifdef __TM_GMTOFF
    char buffer[40] = {};
    TimeUtils::formatDateTime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S",
                              compiled);
    CHECK_STRING("tm conversion retains input calendar fields",
                 "2026-07-01 00:00:00", buffer);
    TimeUtils::formatDateTime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S",
                              offsetCompiled);
    epochTime expectedFormatting(expectedOffsetCompiled + estOffset, estOffset,
                                 epochStart::unix_epoch);
    char      expectedBuffer[40] = {};
    TimeUtils::formatDateTime(expectedBuffer, sizeof(expectedBuffer),
                              "%Y-%m-%d %H:%M:%S", expectedFormatting);
    CHECK_STRING("tm conversion retains its offset only for formatting",
                 expectedBuffer, buffer);
#else
    skip(F("tm conversion retains input offset for formatting"),
         F("this core's tm has no __TM_GMTOFF field"));
#endif  // __TM_GMTOFF
#endif

    tm utcParts = {};
    TimeUtils::fillTimeParts(epochTime(unixUtc), utcParts);
    CHECK_EQUAL("epochTime parts have the UTC hour", 0, utcParts.tm_hour);
    CHECK_EQUAL("epochTime parts have the UTC day", 1, utcParts.tm_mday);

    tm rawParts = {};
    TimeUtils::fillTimeParts(local, estOffset, epochStart::unix_epoch,
                             rawParts);
    CHECK_EQUAL("raw parts have the UTC hour", 0, rawParts.tm_hour);
    CHECK_EQUAL("raw parts have the UTC day", 1, rawParts.tm_mday);

    tm sameA      = makeTimeParts(2026, 7, 1, 0, 0, 0, 0);
    tm sameB      = sameA;
    sameB.tm_wday = 5;
    sameB.tm_yday = 200;
    CHECK_EQUAL("weekday and year-day tm fields are ignored", 1,
                TimeUtils::sameTime(sameA, sameB));

#ifdef __TM_GMTOFF
    tm sameOffsetMetadata          = sameA;
    sameOffsetMetadata.__TM_GMTOFF = estOffset;
    CHECK_EQUAL("tm offset metadata is ignored", 1,
                TimeUtils::sameTime(sameA, sameOffsetMetadata));
#else
    skip(F("tm offset metadata is ignored"),
         F("this core's tm has no __TM_GMTOFF field"));
#endif
}

void testWideGpsTimestamps() {
    printSection(F("Wide timestamp behavior"));
    if (sizeof(timestamp_t) <= sizeof(uint32_t)) {
        skip(F("GPS timestamp beyond UINT32_MAX"),
             F("timestamp_t is only 32 bits on this core"));
        return;
    }

    const timestamp_t gps = static_cast<timestamp_t>(UINT32_MAX) +
        static_cast<timestamp_t>(1000000LL);
    const timestamp_t expectedUnix = gps + EPOCH_UNIX_TO_GPS - 18;
    CHECK_EQUAL("wide GPS conversion counts all leap seconds", expectedUnix,
                TimeUtils::convertEpoch(gps, epochStart::gps_epoch,
                                        epochStart::unix_epoch));
}

void printCoreDetails() {
    Serial.print(F("Core epoch: "));
    printEpochName(TimeUtils::getCoreEpochStart());
    Serial.println();
    Serial.print(F("Core UTC offset (seconds): "));
    Serial.println(TimeUtils::getCoreTimeZone());
    Serial.print(F("timestamp_t width: "));
    Serial.print(static_cast<int>(sizeof(timestamp_t) * 8));
    Serial.print(F(" bits, "));
    Serial.println(static_cast<timestamp_t>(-1) < static_cast<timestamp_t>(0)
                       ? F("signed")
                       : F("unsigned"));
#ifdef __TM_GMTOFF
    Serial.println(F("tm UTC-offset field: available"));
#else
    Serial.println(F("tm UTC-offset field: unavailable"));
#endif
#if EPOCHTIME_ENABLE_STRFTIME
    Serial.println(F("strftime tests: enabled"));
#else
    Serial.println(F("strftime tests: disabled"));
#endif
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println();
    Serial.println(F("========================================"));
    Serial.println(F("EpochTime Arduino edge-case tests"));
    Serial.println(F("========================================"));
    printCoreDetails();

    testOffsetAndEpochConversions();
    testFormattingAndSanity();
    testCoreTimeAndTmConversions();
    testWideGpsTimestamps();

    Serial.println();
    Serial.println(F("========================================"));
    if (failures == 0) {
        Serial.print(F("PASS: all "));
        Serial.print(checks);
        Serial.print(F(" checks passed"));
    } else {
        Serial.print(F("FAIL: "));
        Serial.print(failures);
        Serial.print(F(" of "));
        Serial.print(checks);
        Serial.print(F(" checks failed"));
    }
    if (skips > 0) {
        Serial.print(F("; "));
        Serial.print(skips);
        Serial.print(F(" skipped"));
    }
    Serial.println();
    Serial.println(F("========================================"));
    Serial.flush();
}

void loop() {
    // Tests run once in setup().
}

// cSpell:ignore TINYUSB strftime
