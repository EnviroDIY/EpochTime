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

void checkEqual(const __FlashStringHelper* name, const int64_t& expected,
                const int64_t& actual, int line) {
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
    const timestamp_t unixUtc = static_cast<timestamp_t>(1782864000LL);
    const int32_t     estUtcOffsetSeconds = -5 * 3600L;
    const timestamp_t local               = unixUtc + estUtcOffsetSeconds;

    epochTime localTime(local, estUtcOffsetSeconds, epochStart::unix_epoch);
    CHECK_EQUAL("negative offset normalizes to UTC", unixUtc,
                localTime.getTimestamp());
    CHECK_EQUAL("negative offset round trips", local,
                localTime.getTimestamp(estUtcOffsetSeconds));

    const int32_t indiaUtcOffsetSeconds = 5 * 3600L + 30 * 60L;
    epochTime indiaTime(unixUtc + indiaUtcOffsetSeconds, indiaUtcOffsetSeconds,
                        epochStart::unix_epoch);
    CHECK_EQUAL("positive half-hour offset normalizes to UTC", unixUtc,
                indiaTime.getTimestamp());
    CHECK_EQUAL("positive half-hour offset round trips",
                unixUtc + indiaUtcOffsetSeconds,
                indiaTime.getTimestamp(indiaUtcOffsetSeconds));
    CHECK_EQUAL("equal instants compare equal", 1, localTime == indiaTime);
    CHECK_EQUAL("different instants compare unequal", 1,
                localTime != epochTime(unixUtc + 1));
    CHECK_EQUAL("timezone offset conversion", unixUtc + indiaUtcOffsetSeconds,
                TimeUtils::convertTZOffset(unixUtc, 0, indiaUtcOffsetSeconds));
    CHECK_EQUAL("utility timestamp conversion", unixUtc + indiaUtcOffsetSeconds,
                TimeUtils::getTimestamp(localTime, indiaUtcOffsetSeconds,
                                        epochStart::unix_epoch));

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
    CHECK_EQUAL("same-epoch GPS leap second is preserved", 46828800LL,
                TimeUtils::convertOffsetAndEpoch(
                    static_cast<timestamp_t>(46828800LL), 0,
                    epochStart::gps_epoch, 0, epochStart::gps_epoch));
    CHECK_EQUAL("GPS offset is normalized before leap conversion", 362793600LL,
                TimeUtils::convertOffsetAndEpoch(
                    static_cast<timestamp_t>(46810801LL), estUtcOffsetSeconds,
                    epochStart::gps_epoch, 0, epochStart::unix_epoch));

    epochTime preGpsTime(0, 3600, epochStart::gps_epoch);
    CHECK_EQUAL("pre-epoch GPS offset excludes leap seconds", 315961200LL,
                preGpsTime.getTimestamp());
    CHECK_EQUAL("pre-epoch GPS offset round trips", 0,
                preGpsTime.getTimestamp(3600, epochStart::gps_epoch));
}

void testFixedFormatting() {
    printSection(F("Fixed timestamp formatting"));
    const timestamp_t unixUtc = static_cast<timestamp_t>(1782864000LL);
    const int32_t     estUtcOffsetSeconds   = -5 * 3600L;
    const int32_t     indiaUtcOffsetSeconds = 5 * 3600L + 30 * 60L;
    const epochTime   utcTime(unixUtc);
    const epochTime estTime(unixUtc + estUtcOffsetSeconds, estUtcOffsetSeconds);
    const epochTime indiaTime(unixUtc + indiaUtcOffsetSeconds,
                              indiaUtcOffsetSeconds);
    const epochTime crossingTime(unixUtc + 14 * 3600L, 14 * 3600L);
    char            buffer[26] = {};

    TimeUtils::formatISO8601(buffer, estTime);
    CHECK_STRING("retained-offset ISO formatter", "2026-06-30T19:00:00-05:00",
                 buffer);
    TimeUtils::formatISO8601(buffer, epochTime(unixUtc + 5, 5));
    CHECK_STRING("retained offset uses exact seconds", "", buffer);

    // clang-format off
#define CHECK_FIXED_FORMAT(name, expected, inTime, formatValue, expectedLength) \
    do {                                                                       \
        CHECK_EQUAL(name " length", expectedLength,                            \
                    TimeUtils::formatTimestamp(buffer, sizeof(buffer), inTime, \
                                               formatValue));                  \
        CHECK_STRING(name, expected, buffer);                                  \
    } while (0)
    // clang-format on

    CHECK_FIXED_FORMAT("UTC compact date", "20260701", utcTime,
                       fixedTimeFormat::compact_date, 8);
    CHECK_FIXED_FORMAT("UTC compact date and time", "20260701_000000", utcTime,
                       fixedTimeFormat::compact_date_time, 15);
    CHECK_FIXED_FORMAT("UTC calendar date and time", "2026-07-01 00:00:00",
                       utcTime, fixedTimeFormat::calendar_date_time, 19);
    CHECK_FIXED_FORMAT("UTC-05 compact date", "20260630", estTime,
                       fixedTimeFormat::compact_date, 8);
    CHECK_FIXED_FORMAT("UTC-05 compact date and time", "20260630_190000",
                       estTime, fixedTimeFormat::compact_date_time, 15);
    CHECK_FIXED_FORMAT("UTC-05 calendar date and time", "2026-06-30 19:00:00",
                       estTime, fixedTimeFormat::calendar_date_time, 19);
    CHECK_FIXED_FORMAT("UTC+05:30 compact date", "20260701", indiaTime,
                       fixedTimeFormat::compact_date, 8);
    CHECK_FIXED_FORMAT("UTC+05:30 compact date and time", "20260701_053000",
                       indiaTime, fixedTimeFormat::compact_date_time, 15);
    CHECK_FIXED_FORMAT("UTC+05:30 calendar date and time",
                       "2026-07-01 05:30:00", indiaTime,
                       fixedTimeFormat::calendar_date_time, 19);
    CHECK_FIXED_FORMAT("date-crossing compact date", "20260701", crossingTime,
                       fixedTimeFormat::compact_date, 8);
    CHECK_FIXED_FORMAT("date-crossing compact date and time", "20260701_140000",
                       crossingTime, fixedTimeFormat::compact_date_time, 15);
    CHECK_FIXED_FORMAT("date-crossing calendar date and time",
                       "2026-07-01 14:00:00", crossingTime,
                       fixedTimeFormat::calendar_date_time, 19);
    CHECK_FIXED_FORMAT("leap day", "20240229_235959",
                       epochTime(static_cast<timestamp_t>(1709251199LL)),
                       fixedTimeFormat::compact_date_time, 15);
    CHECK_FIXED_FORMAT("year boundary", "20251231_235959",
                       epochTime(static_cast<timestamp_t>(1767225599LL)),
                       fixedTimeFormat::compact_date_time, 15);
    CHECK_FIXED_FORMAT(
        "earliest sane boundary", "20250101_000000",
        epochTime(static_cast<timestamp_t>(EARLIEST_SANE_UNIX_TIMESTAMP)),
        fixedTimeFormat::compact_date_time, 15);
    CHECK_FIXED_FORMAT(
        "latest sane boundary", "20350101_000000",
        epochTime(static_cast<timestamp_t>(LATEST_SANE_UNIX_TIMESTAMP)),
        fixedTimeFormat::compact_date_time, 15);

#undef CHECK_FIXED_FORMAT

    char exactDate[9] = {};
    CHECK_EQUAL("exact compact-date buffer", 8,
                TimeUtils::formatTimestamp(exactDate, sizeof(exactDate),
                                           utcTime,
                                           fixedTimeFormat::compact_date));
    CHECK_EQUAL("exact compact-date buffer terminator", 0, exactDate[8]);
    char smallBuffer[8] = {'x', 'x', 'x', 'x', 'x', 'x', 'x', '\0'};
    CHECK_EQUAL("undersized fixed-format buffer", 0,
                TimeUtils::formatTimestamp(smallBuffer, sizeof(smallBuffer),
                                           utcTime,
                                           fixedTimeFormat::compact_date));
    CHECK_STRING("undersized fixed-format buffer is empty", "", smallBuffer);
    CHECK_EQUAL("null fixed-format buffer", 0,
                TimeUtils::formatTimestamp(nullptr, 9, utcTime,
                                           fixedTimeFormat::compact_date));

    char offsetBuffer[7] = {};
    CHECK_EQUAL(
        "UTC offset length", 1,
        TimeUtils::formatUtcOffset(offsetBuffer, sizeof(offsetBuffer), 0));
    CHECK_STRING("UTC offset", "Z", offsetBuffer);
    CHECK_EQUAL("negative UTC offset length", 6,
                TimeUtils::formatUtcOffset(offsetBuffer, sizeof(offsetBuffer),
                                           estUtcOffsetSeconds));
    CHECK_STRING("negative UTC offset", "-05:00", offsetBuffer);
    CHECK_EQUAL("fractional UTC offset length", 6,
                TimeUtils::formatUtcOffset(offsetBuffer, sizeof(offsetBuffer),
                                           indiaUtcOffsetSeconds));
    CHECK_STRING("fractional UTC offset", "+05:30", offsetBuffer);
    CHECK_EQUAL(
        "partial-minute UTC offset is rejected", 0,
        TimeUtils::formatUtcOffset(offsetBuffer, sizeof(offsetBuffer), 1));
    CHECK_STRING("rejected UTC offset buffer is empty", "", offsetBuffer);
}

void testFormattingAndSanity() {
    printSection(F("Formatting and sanity boundaries"));
    const timestamp_t unixUtc = static_cast<timestamp_t>(1782864000LL);
    const int32_t     estUtcOffsetSeconds = -5 * 3600L;
    const timestamp_t local               = unixUtc + estUtcOffsetSeconds;
    epochTime localTime(local, estUtcOffsetSeconds, epochStart::unix_epoch);

    char buffer[40] = {};
    TimeUtils::formatISO8601(buffer, local, estUtcOffsetSeconds,
                             epochStart::unix_epoch);
    CHECK_STRING("raw ISO timestamp retains UTC-5 fields",
                 "2026-06-30T19:00:00-05:00", buffer);
    TimeUtils::formatISO8601(buffer, localTime, 14 * 3600L);
    CHECK_STRING("ISO conversion crosses into UTC+14",
                 "2026-07-01T14:00:00+14:00", buffer);
    TimeUtils::formatISO8601(buffer, localTime, -12 * 3600L);
    CHECK_STRING("ISO conversion crosses into UTC-12",
                 "2026-06-30T12:00:00-12:00", buffer);
    TimeUtils::formatISO8601(buffer, localTime, 5 * 3600L + 30 * 60L);
    CHECK_STRING("ISO conversion supports UTC+05:30",
                 "2026-07-01T05:30:00+05:30", buffer);
    TimeUtils::formatISO8601(buffer, localTime, -5);
    CHECK_STRING("hour-valued ISO offset is rejected", "", buffer);

#if EPOCHTIME_ENABLE_STRING_FORMATTING
    String rawISO = TimeUtils::formatISO8601(local, estUtcOffsetSeconds,
                                             epochStart::unix_epoch);
    CHECK_STRING("raw ISO String overload", "2026-06-30T19:00:00-05:00",
                 rawISO.c_str());
    String objectISO = TimeUtils::formatISO8601(localTime, 14 * 3600L);
    CHECK_STRING("epochTime ISO String overload", "2026-07-01T14:00:00+14:00",
                 objectISO.c_str());
#endif

    TimeUtils::formatISO8601(buffer, static_cast<timestamp_t>(1709251199LL), 0,
                             epochStart::unix_epoch);
    CHECK_STRING("leap-day final second formats correctly",
                 "2024-02-29T23:59:59+00:00", buffer);
    TimeUtils::formatISO8601(buffer, static_cast<timestamp_t>(1767225599LL), 0,
                             epochStart::unix_epoch);
    CHECK_STRING("year-boundary final second formats correctly",
                 "2025-12-31T23:59:59+00:00", buffer);

#if EPOCHTIME_ENABLE_STRFTIME
    TimeUtils::formatDateTime(buffer, "%Y-%m-%d %H:%M:%S", localTime);
    CHECK_STRING("epochTime unchecked-buffer formatter", "2026-06-30 19:00:00",
                 buffer);
    TimeUtils::formatDateTime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S",
                              localTime);
    CHECK_STRING("custom formatter retains a negative offset",
                 "2026-06-30 19:00:00", buffer);
#if EPOCHTIME_ENABLE_STRING_FORMATTING
    String objectDateTime = TimeUtils::formatDateTime("%Y-%m-%d %H:%M:%S",
                                                      localTime);
    CHECK_STRING("epochTime String formatter", "2026-06-30 19:00:00",
                 objectDateTime.c_str());
#endif

    const int32_t indiaUtcOffsetSeconds = 5 * 3600L + 30 * 60L;
    epochTime indiaTime(unixUtc + indiaUtcOffsetSeconds, indiaUtcOffsetSeconds,
                        epochStart::unix_epoch);
    TimeUtils::formatDateTime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S",
                              indiaTime);
    CHECK_STRING("custom formatter retains a half-hour offset",
                 "2026-07-01 05:30:00", buffer);

    TimeUtils::formatDateTime(buffer, "%Y-%m-%d %H:%M:%S", local,
                              epochStart::unix_epoch);
    CHECK_STRING("raw unchecked-buffer formatter", "2026-06-30 19:00:00",
                 buffer);
    TimeUtils::formatDateTime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S",
                              local, epochStart::unix_epoch);
    CHECK_STRING("raw custom formatter retains calendar fields",
                 "2026-06-30 19:00:00", buffer);
#if EPOCHTIME_ENABLE_STRING_FORMATTING
    String rawDateTime = TimeUtils::formatDateTime("%Y-%m-%d %H:%M:%S", local,
                                                   epochStart::unix_epoch);
    CHECK_STRING("raw String formatter", "2026-06-30 19:00:00",
                 rawDateTime.c_str());
#endif

    char         smallBuffer[8]    = {'x', 'x', 'x', 'x', 'x', 'x', 'x', '\0'};
    const size_t smallBufferLength = TimeUtils::formatDateTime(
        smallBuffer, sizeof(smallBuffer), "%Y-%m-%d", localTime);
    CHECK_EQUAL("small format buffer matches the reported result", 1,
                smallBufferLength == 0 ? smallBuffer[0] == '\0'
                                       : smallBuffer[0] != 'x');
    CHECK_EQUAL("small format buffer is null-terminated", 0,
                smallBuffer[sizeof(smallBuffer) - 1]);

    const char longFormat[] = "123456789012345678901234567890123456789012345678"
                              "9012345678901234567890";
    char       longBuffer[sizeof(longFormat)] = {};
    TimeUtils::formatDateTime(longBuffer, sizeof(longBuffer), longFormat,
                              localTime);
    CHECK_STRING("long custom formatter output", longFormat, longBuffer);
#else
    skip(F("custom date/time formatting"),
         F("EPOCHTIME_ENABLE_STRFTIME is disabled"));
#endif

    const timestamp_t earliestLocal =
        static_cast<timestamp_t>(EARLIEST_SANE_UNIX_TIMESTAMP) +
        estUtcOffsetSeconds;
    CHECK_EQUAL("earliest sane instant is included", 1,
                TimeUtils::isTimeSane(earliestLocal, estUtcOffsetSeconds,
                                      epochStart::unix_epoch));
    CHECK_EQUAL(
        "hour-valued sane offset is interpreted as seconds", 0,
        TimeUtils::isTimeSane(earliestLocal, -5, epochStart::unix_epoch));
    CHECK_EQUAL("second before sane range is rejected", 0,
                TimeUtils::isTimeSane(earliestLocal - 1, estUtcOffsetSeconds,
                                      epochStart::unix_epoch));
    CHECK_EQUAL("latest sane instant is included", 1,
                TimeUtils::isTimeSane(
                    static_cast<timestamp_t>(LATEST_SANE_UNIX_TIMESTAMP), 0,
                    epochStart::unix_epoch));
    CHECK_EQUAL("second after sane range is rejected", 0,
                TimeUtils::isTimeSane(
                    static_cast<timestamp_t>(LATEST_SANE_UNIX_TIMESTAMP) + 1, 0,
                    epochStart::unix_epoch));
    CHECK_EQUAL("epochTime sanity overload", 1,
                TimeUtils::isTimeSane(epochTime(unixUtc)));

#if EPOCHTIME_ENABLE_STRING_FORMATTING
    CHECK_STRING("Unix epoch name", "Unix",
                 TimeUtils::printEpochName(epochStart::unix_epoch).c_str());
    CHECK_STRING("Y2K epoch name", "Y2K",
                 TimeUtils::printEpochName(epochStart::y2k_epoch).c_str());
    CHECK_STRING("GPS epoch name", "GPS",
                 TimeUtils::printEpochName(epochStart::gps_epoch).c_str());
    CHECK_STRING("NIST epoch name", "NIST",
                 TimeUtils::printEpochName(epochStart::nist_epoch).c_str());
    CHECK_STRING("Unix epoch start", "1970-01-01T00:00:00Z",
                 TimeUtils::printEpochStart(epochStart::unix_epoch).c_str());
    CHECK_STRING("Y2K epoch start", "2000-01-01T00:00:00Z",
                 TimeUtils::printEpochStart(epochStart::y2k_epoch).c_str());
    CHECK_STRING("GPS epoch start", "1980-01-06T00:00:00Z",
                 TimeUtils::printEpochStart(epochStart::gps_epoch).c_str());
    CHECK_STRING("NIST epoch start", "1900-01-01T00:00:00Z",
                 TimeUtils::printEpochStart(epochStart::nist_epoch).c_str());
#endif
}

void testCoreTimeAndTmConversions() {
    printSection(F("Core time_t and tm conversions"));
    const timestamp_t unixUtc = static_cast<timestamp_t>(1782864000LL);
    const int32_t     estUtcOffsetSeconds = -5 * 3600L;
    const timestamp_t local               = unixUtc + estUtcOffsetSeconds;
    const epochStart  coreEpoch           = TimeUtils::getCoreEpochStart();
    const int32_t     coreOffset          = TimeUtils::getCoreTimeZone();
    const timestamp_t expectedCore =
        TimeUtils::convertEpoch(unixUtc, epochStart::unix_epoch, coreEpoch);

    CHECK_EQUAL("epochTime converts to the core epoch", expectedCore,
                TimeUtils::getTimeT(epochTime(unixUtc)));
    CHECK_EQUAL("local timestamp converts to the core epoch", expectedCore,
                TimeUtils::getTimeT(local, estUtcOffsetSeconds,
                                    epochStart::unix_epoch));

    tm                parts            = makeTimeParts(2026, 7, 1, 0, 0, 0, 0);
    const timestamp_t expectedCompiled = unixUtc - coreOffset;
    epochTime         compiled         = TimeUtils::tmToEpochTime(parts);
    CHECK_EQUAL("tm fields compile to the expected UTC instant",
                expectedCompiled, compiled.getTimestamp());

#ifdef __TM_GMTOFF
    tm offsetParts = makeTimeParts(2026, 6, 30, 19, 0, 0, estUtcOffsetSeconds);
    epochTime         offsetCompiled = TimeUtils::tmToEpochTime(offsetParts);
    const timestamp_t expectedOffsetCompiled = unixUtc + estUtcOffsetSeconds -
        coreOffset;
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
    epochTime expectedFormatting(expectedOffsetCompiled + estUtcOffsetSeconds,
                                 estUtcOffsetSeconds, epochStart::unix_epoch);
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
    CHECK_EQUAL("epochTime parts conversion succeeds", 1,
                TimeUtils::fillTimeParts(epochTime(unixUtc), utcParts));
    CHECK_EQUAL("epochTime parts have the UTC hour", 0, utcParts.tm_hour);
    CHECK_EQUAL("epochTime parts have the UTC day", 1, utcParts.tm_mday);

    tm offsetParts = {};
    CHECK_EQUAL("offset epochTime parts conversion succeeds", 1,
                TimeUtils::fillTimeParts(epochTime(unixUtc),
                                         estUtcOffsetSeconds, offsetParts));
    CHECK_EQUAL("offset epochTime parts have the local hour", 19,
                offsetParts.tm_hour);
    CHECK_EQUAL("offset epochTime parts have the local day", 30,
                offsetParts.tm_mday);

    tm rawParts = {};
    CHECK_EQUAL("raw parts conversion succeeds", 1,
                TimeUtils::fillTimeParts(local, estUtcOffsetSeconds,
                                         epochStart::unix_epoch, rawParts));
    CHECK_EQUAL("raw parts have the UTC hour", 0, rawParts.tm_hour);
    CHECK_EQUAL("raw parts have the UTC day", 1, rawParts.tm_mday);

    tm sameA      = makeTimeParts(2026, 7, 1, 0, 0, 0, 0);
    tm sameB      = sameA;
    sameB.tm_wday = 5;
    sameB.tm_yday = 200;
    CHECK_EQUAL("weekday and year-day tm fields are ignored", 1,
                TimeUtils::sameTime(sameA, sameB));

    tm differentField     = sameA;
    differentField.tm_sec = 1;
    CHECK_EQUAL("different calendar fields compare unequal", 0,
                TimeUtils::sameTime(sameA, differentField));

    tm daylightA       = sameA;
    daylightA.tm_isdst = 1;
    tm daylightB       = daylightA;
    CHECK_EQUAL("equal non-negative DST fields compare equal", 1,
                TimeUtils::sameTime(daylightA, daylightB));
    CHECK_EQUAL("different non-negative DST fields compare unequal", 0,
                TimeUtils::sameTime(sameA, daylightA));

    tm unknownDst       = sameA;
    unknownDst.tm_isdst = -1;
    CHECK_EQUAL("unknown first DST field is incomparable", 0,
                TimeUtils::sameTime(unknownDst, sameA));
    CHECK_EQUAL("unknown second DST field is incomparable", 0,
                TimeUtils::sameTime(sameA, unknownDst));
    CHECK_EQUAL("two unknown DST fields are incomparable", 0,
                TimeUtils::sameTime(unknownDst, unknownDst));

#ifdef __TM_GMTOFF
    tm sameOffsetMetadata          = sameA;
    sameOffsetMetadata.__TM_GMTOFF = estUtcOffsetSeconds;
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

    const timestamp_t gps = static_cast<timestamp_t>(
        static_cast<uint64_t>(UINT32_MAX) + 1000000ULL);
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

    TimeUtils::begin();

    Serial.println();
    Serial.println(F("========================================"));
    Serial.println(F("EpochTime Arduino edge-case tests"));
    Serial.println(F("========================================"));
    printCoreDetails();

    testOffsetAndEpochConversions();
    testFixedFormatting();
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
