#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "EpochTime.h"

namespace {

int failures = 0;
int checks   = 0;

void checkEqual(const char* name, int64_t expected, int64_t actual, int line) {
    ++checks;
    if (expected != actual) {
        ++failures;
        printf("FAIL line %d: %s: expected %lld, got %lld\n", line, name,
               static_cast<long long>(expected),
               static_cast<long long>(actual));
    }
}

void checkString(const char* name, const char* expected, const char* actual,
                 int line) {
    ++checks;
    if (strcmp(expected, actual) != 0) {
        ++failures;
        printf("FAIL line %d: %s: expected \"%s\", got \"%s\"\n", line, name,
               expected, actual);
    }
}

#define CHECK_EQUAL(name, expected, actual)          \
    checkEqual(name, static_cast<int64_t>(expected), \
               static_cast<int64_t>(actual), __LINE__)
#define CHECK_STRING(name, expected, actual) \
    checkString(name, expected, actual, __LINE__)

void configureTestTimezone() {
#ifdef _WIN32
    _putenv_s("TZ", "EST5");
    _tzset();
#else
    setenv("TZ", "EST5", 1);
    tzset();
#endif
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
    const timestamp_t unixUtc = static_cast<timestamp_t>(1782864000LL);
    const int32_t     estUtcOffsetSeconds = -5 * 3600L;
    const timestamp_t local               = unixUtc + estUtcOffsetSeconds;

    epochTime localTime(local, estUtcOffsetSeconds, epochStart::unix_epoch);
    CHECK_EQUAL("constructor normalizes UTC", unixUtc,
                localTime.getTimestamp());
    CHECK_EQUAL("constructor retains local representation", local,
                localTime.getTimestamp(estUtcOffsetSeconds));

    const int32_t istUtcOffsetSeconds = 5 * 3600L + 30 * 60L;
    epochTime     indiaTime(unixUtc + istUtcOffsetSeconds, istUtcOffsetSeconds,
                            epochStart::unix_epoch);
    CHECK_EQUAL("positive half-hour offset normalizes UTC", unixUtc,
                indiaTime.getTimestamp());
    CHECK_EQUAL("positive half-hour offset retains local representation",
                unixUtc + istUtcOffsetSeconds,
                indiaTime.getTimestamp(istUtcOffsetSeconds));

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
    CHECK_EQUAL("Unix to GPS", gps,
                TimeUtils::convertEpoch(unixUtc, epochStart::unix_epoch,
                                        epochStart::gps_epoch));
    CHECK_EQUAL("GPS round trip", unixUtc,
                TimeUtils::convertEpoch(gps, epochStart::gps_epoch,
                                        epochStart::unix_epoch));

    CHECK_EQUAL("GPS offset normalized before leap conversion", 362793600LL,
                TimeUtils::convertOffsetAndEpoch(
                    static_cast<timestamp_t>(46810801LL), estUtcOffsetSeconds,
                    epochStart::gps_epoch, 0, epochStart::unix_epoch));
}

void testFormattingAndSanity() {
    const timestamp_t unixUtc = static_cast<timestamp_t>(1782864000LL);
    const int32_t     estUtcOffsetSeconds = -5 * 3600L;
    const timestamp_t local               = unixUtc + estUtcOffsetSeconds;
    epochTime localTime(local, estUtcOffsetSeconds, epochStart::unix_epoch);

    char buffer[40] = {};
    TimeUtils::formatISO8601(buffer, local, -5, epochStart::unix_epoch);
    CHECK_STRING("raw ISO offset units", "2026-06-30T19:00:00-05:00", buffer);

    CHECK_EQUAL("safe custom formatter length", 19,
                TimeUtils::formatDateTime(buffer, sizeof(buffer),
                                          "%Y-%m-%d %H:%M:%S", localTime));
    CHECK_STRING("epochTime custom formatter offset", "2026-06-30 19:00:00",
                 buffer);

    const int32_t istUtcOffsetSeconds = 5 * 3600L + 30 * 60L;
    epochTime     indiaTime(unixUtc + istUtcOffsetSeconds, istUtcOffsetSeconds,
                            epochStart::unix_epoch);
    TimeUtils::formatDateTime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S",
                              indiaTime);
    CHECK_STRING("positive half-hour formatter offset", "2026-07-01 05:30:00",
                 buffer);

    CHECK_EQUAL("raw custom formatter length", 19,
                TimeUtils::formatDateTime(buffer, sizeof(buffer),
                                          "%Y-%m-%d %H:%M:%S", local,
                                          epochStart::unix_epoch));
    CHECK_STRING("raw custom formatter retains fields", "2026-06-30 19:00:00",
                 buffer);

    char smallBuffer[8] = {'x', 'x', 'x', 'x', 'x', 'x', 'x', '\0'};
    CHECK_EQUAL("small buffer reports failure", 0,
                TimeUtils::formatDateTime(smallBuffer, sizeof(smallBuffer),
                                          "%Y-%m-%d", localTime));
    CHECK_EQUAL("small buffer is empty", 0, smallBuffer[0]);

    const timestamp_t earliestLocal =
        static_cast<timestamp_t>(EARLIEST_SANE_UNIX_TIMESTAMP) +
        estUtcOffsetSeconds;
    CHECK_EQUAL(
        "sane offset converted from hours", 1,
        TimeUtils::isTimeSane(earliestLocal, -5, epochStart::unix_epoch));
    CHECK_EQUAL(
        "one second before sane range", 0,
        TimeUtils::isTimeSane(earliestLocal - 1, -5, epochStart::unix_epoch));
}

void testCoreTimeAndTmConversions() {
    const timestamp_t unixUtc = static_cast<timestamp_t>(1782864000LL);
    const int32_t     estUtcOffsetSeconds = -5 * 3600L;
    const timestamp_t local               = unixUtc + estUtcOffsetSeconds;

    CHECK_EQUAL("host core epoch",
                static_cast<timestamp_t>(epochStart::unix_epoch),
                static_cast<timestamp_t>(TimeUtils::getCoreEpochStart()));
    CHECK_EQUAL("detected core offset sign", estUtcOffsetSeconds,
                TimeUtils::getCoreTimeZone());

    epochTime utcTime(unixUtc, 0, epochStart::unix_epoch);
    CHECK_EQUAL("epochTime to scalar time_t", unixUtc,
                TimeUtils::getTimeT(utcTime));
    CHECK_EQUAL("local timestamp to scalar time_t", unixUtc,
                TimeUtils::getTimeT(local, estUtcOffsetSeconds,
                                    epochStart::unix_epoch));

    tm localParts = makeTimeParts(2026, 6, 30, 19, 0, 0, estUtcOffsetSeconds);
    epochTime compiled = TimeUtils::tmToEpochTime(localParts);
    CHECK_EQUAL("tm to UTC instant", unixUtc, compiled.getTimestamp());

    char buffer[40] = {};
    TimeUtils::formatDateTime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S",
                              compiled);
#ifdef __TM_GMTOFF
    CHECK_STRING("tm offset is retained only for formatting",
                 "2026-06-30 19:00:00", buffer);

    tm sameFieldsUtcOffset          = localParts;
    sameFieldsUtcOffset.__TM_GMTOFF = 0;
    CHECK_EQUAL("tm offset does not change the compiled instant",
                compiled.getTimestamp(),
                TimeUtils::tmToEpochTime(sameFieldsUtcOffset).getTimestamp());
#else
    CHECK_STRING("tm without an offset formats as UTC", "2026-07-01 00:00:00",
                 buffer);
#endif

    tm utcParts = {};
    TimeUtils::fillTimeParts(compiled, utcParts);
    CHECK_EQUAL("epochTime parts are UTC hour", 0, utcParts.tm_hour);
    CHECK_EQUAL("epochTime parts are UTC day", 1, utcParts.tm_mday);

    tm rawParts = {};
    TimeUtils::fillTimeParts(local, estUtcOffsetSeconds, epochStart::unix_epoch,
                             rawParts);
    CHECK_EQUAL("raw parts are UTC hour", 0, rawParts.tm_hour);
    CHECK_EQUAL("raw parts are UTC day", 1, rawParts.tm_mday);

    tm sameA      = makeTimeParts(2026, 7, 1, 0, 0, 0, 0);
    tm sameB      = sameA;
    sameB.tm_wday = 5;
    sameB.tm_yday = 200;
    CHECK_EQUAL("derived tm fields ignored", 1,
                TimeUtils::sameTime(sameA, sameB));

#ifdef __TM_GMTOFF
    tm sameOffsetMetadata          = sameA;
    sameOffsetMetadata.__TM_GMTOFF = estUtcOffsetSeconds;
    CHECK_EQUAL("tm offset metadata is ignored", 1,
                TimeUtils::sameTime(sameA, sameOffsetMetadata));
#endif
}

void testWideGpsTimestamps() {
    if (sizeof(timestamp_t) <= sizeof(uint32_t)) { return; }

    const timestamp_t gps = static_cast<timestamp_t>(UINT32_MAX) + 1000000LL;
    const timestamp_t expectedUnix = gps + EPOCH_UNIX_TO_GPS - 18;
    CHECK_EQUAL("wide GPS leap count", expectedUnix,
                TimeUtils::convertEpoch(gps, epochStart::gps_epoch,
                                        epochStart::unix_epoch));
}

}  // namespace

int main() {
    configureTestTimezone();
    testOffsetAndEpochConversions();
    testFormattingAndSanity();
    testCoreTimeAndTmConversions();
    testWideGpsTimestamps();

    if (failures == 0) {
        printf("PASS: %d checks\n", checks);
        return 0;
    }

    printf("FAIL: %d of %d checks failed\n", failures, checks);
    return 1;
}
