/**
 * @file EpochTime.cpp
 * @copyright Stroud Water Research Center
 * Part of the EnviroDIY EpochTime library for Arduino.
 * This library is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Contains implementations for the epochTime class and TimeUtils class.
 */
#include "EpochTime.h"

#include <limits.h>

// Initialize the array for the leap seconds - taken from the defines
const uint32_t TimeUtils::leapSeconds[NUMBER_LEAP_SECONDS] = LEAP_SECONDS;

// Initialize the processor epoch
epochStart TimeUtils::_core_epoch = epochStart::y2k_epoch;
// Initialize the processor timezone offset
int32_t TimeUtils::_coreUtcOffsetSeconds = 0;
// Initialize the flag tracking initialization state
bool TimeUtils::_initialized = false;


epochTime::epochTime(timestamp_t timestamp, int32_t utcOffsetSeconds,
                     epochStart epoch) {
    // Convert the input timestamp from the input epoch and offset to Unix with
    // 0 offset and store that
    _unixUTCTimestamp = TimeUtils::convertOffsetAndEpoch(
        timestamp, utcOffsetSeconds, epoch, 0, epochStart::unix_epoch);
    // Also store the original UTC offset for later use
    _utcOffsetSeconds = utcOffsetSeconds;
}

timestamp_t epochTime::getTimestamp(int32_t    out_utcOffsetSeconds,
                                    epochStart out_epoch) {
    return TimeUtils::getTimestamp(*this, out_utcOffsetSeconds, out_epoch);
}

// Format an epoch time as ISO8601 without using strftime(), snprintf(), or
// intermediate String objects.  The caller must provide at least 26 bytes.
void TimeUtils::formatISO8601(char* buffer, epochTime in_time,
                              int8_t utcOffsetHours) {
    _ensureInitialized();
    // convert the offset in hours to an offset in seconds
    const int32_t outputUtcOffsetSeconds =
        static_cast<int32_t>(utcOffsetHours) * 3600L;
    // Get a single number timestamp representing the input time in the
    // processor core's epoch and the requested output offset before using
    // gmtime().  The gmtime function expects the time_t to be in UTC and
    // returns a tm struct representing UTC time; the important point being
    // that gmtime does not apply any timezone offset to the input time.   So if
    // we want the final time to be in a different offset, apply that offset
    // first ands gmtime() will be none-the-wiser about the offset.
    // NOTE: gmtime requires a true time_t (in whatever size the core uses) as
    // input!
    time_t t = static_cast<time_t>(TimeUtils::getTimestamp(
        in_time, outputUtcOffsetSeconds, TimeUtils::_core_epoch));

    // Now use gmtime_r() to get the broken-down UTC time corresponding to the
    // adjusted timestamp.
    tm timeParts;
    gmtime_r(&t, &timeParts);

    const int16_t year   = static_cast<int16_t>(timeParts.tm_year + 1900);
    uint8_t       month  = static_cast<uint8_t>(timeParts.tm_mon + 1);
    uint8_t       day    = static_cast<uint8_t>(timeParts.tm_mday);
    uint8_t       hour   = static_cast<uint8_t>(timeParts.tm_hour);
    uint8_t       minute = static_cast<uint8_t>(timeParts.tm_min);
    uint8_t       second = static_cast<uint8_t>(timeParts.tm_sec);

    // YYYY-MM-DDThh:mm:ss
    buffer[0]  = static_cast<char>('0' + (year / 1000) % 10);
    buffer[1]  = static_cast<char>('0' + (year / 100) % 10);
    buffer[2]  = static_cast<char>('0' + (year / 10) % 10);
    buffer[3]  = static_cast<char>('0' + year % 10);
    buffer[4]  = '-';
    buffer[5]  = static_cast<char>('0' + month / 10);
    buffer[6]  = static_cast<char>('0' + month % 10);
    buffer[7]  = '-';
    buffer[8]  = static_cast<char>('0' + day / 10);
    buffer[9]  = static_cast<char>('0' + day % 10);
    buffer[10] = 'T';
    buffer[11] = static_cast<char>('0' + hour / 10);
    buffer[12] = static_cast<char>('0' + hour % 10);
    buffer[13] = ':';
    buffer[14] = static_cast<char>('0' + minute / 10);
    buffer[15] = static_cast<char>('0' + minute % 10);
    buffer[16] = ':';
    buffer[17] = static_cast<char>('0' + second / 10);
    buffer[18] = static_cast<char>('0' + second % 10);

    // The public API currently accepts whole-hour offsets. Keep the output
    // construction explicit rather than invoking printf-family formatting.
    int16_t utcOffsetMinutes = static_cast<int16_t>(utcOffsetHours) * 60;
    char    sign             = '+';
    if (utcOffsetMinutes < 0) {
        sign             = '-';
        utcOffsetMinutes = -utcOffsetMinutes;
    }
    const uint8_t absoluteUtcOffsetHours =
        static_cast<uint8_t>(utcOffsetMinutes / 60);
    const uint8_t remainingUtcOffsetMinutes =
        static_cast<uint8_t>(utcOffsetMinutes % 60);

    buffer[19] = sign;
    buffer[20] = static_cast<char>('0' + absoluteUtcOffsetHours / 10);
    buffer[21] = static_cast<char>('0' + absoluteUtcOffsetHours % 10);
    buffer[22] = ':';
    buffer[23] = static_cast<char>('0' + remainingUtcOffsetMinutes / 10);
    buffer[24] = static_cast<char>('0' + remainingUtcOffsetMinutes % 10);
    buffer[25] = '\0';
}

void TimeUtils::formatISO8601(char* buffer, timestamp_t epochSeconds,
                              int8_t utcOffsetHours, epochStart epoch) {
    const int32_t utcOffsetSeconds = static_cast<int32_t>(utcOffsetHours) *
        3600L;
    formatISO8601(buffer, epochTime(epochSeconds, utcOffsetSeconds, epoch),
                  utcOffsetHours);
}

#if EPOCHTIME_ENABLE_STRING_FORMATTING
String TimeUtils::formatISO8601(timestamp_t epochSeconds, int8_t utcOffsetHours,
                                epochStart epoch) {
    char buffer[26];
    formatISO8601(buffer, epochSeconds, utcOffsetHours, epoch);
    return String(buffer);
}

String TimeUtils::formatISO8601(epochTime in_time, int8_t utcOffsetHours) {
    char buffer[26];
    formatISO8601(buffer, in_time, utcOffsetHours);
    return String(buffer);
}
#endif

#if EPOCHTIME_ENABLE_STRFTIME
void TimeUtils::formatDateTime(char* buffer, const char* fmt,
                               timestamp_t epochSeconds, epochStart epoch) {
    formatDateTime(buffer, 39, fmt, epochTime(epochSeconds, 0, epoch));
}

size_t TimeUtils::formatDateTime(char* buffer, size_t bufferSize,
                                 const char* fmt, timestamp_t epochSeconds,
                                 epochStart epoch) {
    return formatDateTime(buffer, bufferSize, fmt,
                          epochTime(epochSeconds, 0, epoch));
}
#if EPOCHTIME_ENABLE_STRING_FORMATTING
String TimeUtils::formatDateTime(const char* fmt, timestamp_t epochSeconds,
                                 epochStart epoch) {
    return formatDateTime(fmt, epochTime(epochSeconds, 0, epoch));
}
#endif
void TimeUtils::formatDateTime(char* buffer, const char* fmt,
                               epochTime in_time) {
    formatDateTime(buffer, 39, fmt, in_time);
}

size_t TimeUtils::formatDateTime(char* buffer, size_t bufferSize,
                                 const char* fmt, epochTime in_time) {
    if (buffer == nullptr || bufferSize == 0) { return 0; }
    if (fmt == nullptr) {
        buffer[0] = '\0';
        return 0;
    }

    _ensureInitialized();
    // Get a single number timestamp representing the input time in the
    // processor core's epoch and the processor's UTC offset.
    time_t t = static_cast<time_t>(TimeUtils::getTimestamp(
        in_time, in_time._utcOffsetSeconds, TimeUtils::_core_epoch));

    // Now use gmtime() to get the broken-down UTC time corresponding to the
    // adjusted timestamp.   The gmtime function expects the time_t to be in UTC
    // and returns a tm struct representing UTC time - the important point being
    // that gmtime does not apply any timezone offset to the input time. BUT,
    // we're not giving gmtime() UTC here, we're giving it local time, although
    // its none-the-wiser.  This means that the output tm struct will represent
    // the local time corresponding to the adjusted timestamp and any formatting
    // done with strftime that expects the input to be in local time will work
    // correctly.
    // NOTE: gmtime requires a true time_t (in whatever size the core uses) as
    // input!
    tm* tmp = gmtime(&t);
    if (tmp == nullptr) {
        buffer[0] = '\0';
        return 0;
    }

    // create a temporary buffer to format into
    // We use the temporary buffer to guard against a buffer that is too small.
    // The return of strftime is inconsistent.  The expected POSIX behavior is
    // to return 0 if the buffer is to small, but the Arduino core may simply
    // truncate the output without indicating an error.
    char temp[64] = {};
    // use strftime (from time.h) to format the time
    size_t length = strftime(temp, sizeof(temp), fmt, tmp);

    // Normalize implementations that return strlen+1.
    const size_t actualLength = strlen(temp);
    if ((length != actualLength && length != actualLength + 1) ||
        actualLength == 0 || actualLength >= bufferSize) {
        buffer[0] = '\0';
        return 0;
    }

    strncpy(buffer, temp, bufferSize);
    buffer[bufferSize - 1] = '\0';  // Ensure null-termination
    return actualLength;
}
#if EPOCHTIME_ENABLE_STRING_FORMATTING
String TimeUtils::formatDateTime(const char* fmt, epochTime in_time) {
    // 38+1 for the longest common English format:
    // Wednesday, September 30, 2026 23:59:59
    char buffer[39] = {};
    if (formatDateTime(buffer, sizeof(buffer), fmt, in_time) == 0) {
        return String();
    }
    return String(buffer);
}
#endif
#endif  // EPOCHTIME_ENABLE_STRFTIME

#if EPOCHTIME_ENABLE_STRING_FORMATTING
String TimeUtils::printEpochName(epochStart epoch) {
    switch (epoch) {
        case epochStart::unix_epoch: return "Unix";
        case epochStart::y2k_epoch: return "Y2K";
        case epochStart::gps_epoch: return "GPS";
        case epochStart::nist_epoch: return "NIST";
        default: return "UNKNOWN";
    }
}

String TimeUtils::printEpochStart(epochStart epoch) {
    switch (epoch) {
        case epochStart::unix_epoch: return "1970-01-01T00:00:00Z";
        case epochStart::y2k_epoch: return "2000-01-01T00:00:00Z";
        case epochStart::gps_epoch: return "1980-01-06T00:00:00Z";
        case epochStart::nist_epoch: return "1900-01-01T00:00:00Z";
        default: return "UNKNOWN";
    }
}
#endif

bool TimeUtils::isTimeSane(timestamp_t ts, int8_t utcOffsetHours,
                           epochStart epoch) {
    const int32_t utcOffsetSeconds = static_cast<int32_t>(utcOffsetHours) *
        3600L;
    return isTimeSane(epochTime(ts, utcOffsetSeconds, epoch));
}
bool TimeUtils::isTimeSane(epochTime in_time) {
    _ensureInitialized();
    if (in_time._unixUTCTimestamp < EARLIEST_SANE_UNIX_TIMESTAMP ||
        in_time._unixUTCTimestamp > LATEST_SANE_UNIX_TIMESTAMP) {
        return false;
    } else {
        return true;
    }
}

timestamp_t TimeUtils::convertEpoch(timestamp_t in_timestamp,
                                    epochStart in_epoch, epochStart out_epoch) {
    if (in_epoch == out_epoch) { return in_timestamp; }

    // Normalize the input to Unix epoch first, then convert Unix epoch to the
    // requested output epoch. This keeps the conversion matrix small while
    // preserving the GPS leap-second handling in unix2gps()/gps2unix().
    timestamp_t unixTimestamp;
    switch (in_epoch) {
        case epochStart::unix_epoch: {
            unixTimestamp = in_timestamp;
            break;
        }
        case epochStart::y2k_epoch: {
            unixTimestamp = in_timestamp + EPOCH_UNIX_TO_Y2K;
            break;
        }
        case epochStart::gps_epoch: {
            unixTimestamp = TimeUtils::gps2unix(in_timestamp);
            break;
        }
        case epochStart::nist_epoch: {
            unixTimestamp = in_timestamp - EPOCH_NIST_TO_UNIX;
            break;
        }
        default: {
            return in_timestamp;
        }
    }

    switch (out_epoch) {
        case epochStart::unix_epoch: return unixTimestamp;
        case epochStart::y2k_epoch: return unixTimestamp - EPOCH_UNIX_TO_Y2K;
        case epochStart::gps_epoch: return TimeUtils::unix2gps(unixTimestamp);
        case epochStart::nist_epoch: return unixTimestamp + EPOCH_NIST_TO_UNIX;
        default: return in_timestamp;
    }
}

timestamp_t TimeUtils::convertTZOffset(timestamp_t in_timestamp,
                                       int32_t     in_utcOffsetSeconds,
                                       int32_t     out_utcOffsetSeconds) {
    return in_timestamp + (out_utcOffsetSeconds - in_utcOffsetSeconds);
}

timestamp_t TimeUtils::convertOffsetAndEpoch(timestamp_t in_timestamp,
                                             int32_t     in_utcOffsetSeconds,
                                             epochStart  in_epoch,
                                             int32_t     out_utcOffsetSeconds,
                                             epochStart  out_epoch) {
    // First convert to UTC timestamp, then convert between epochs, and finally
    // apply the output timezone offset. If we convert the epoch in the local
    // time zone first, the conversion to the GPS epoch could be incorrect due
    // to the offset screwing up the leap seconds.
    const timestamp_t utcTimestamp = convertTZOffset(in_timestamp,
                                                     in_utcOffsetSeconds, 0);
    const timestamp_t convertedTimestamp = convertEpoch(utcTimestamp, in_epoch,
                                                        out_epoch);
    return convertTZOffset(convertedTimestamp, 0, out_utcOffsetSeconds);
}

timestamp_t TimeUtils::getTimestamp(epochTime  in_time,
                                    int32_t    out_utcOffsetSeconds,
                                    epochStart out_epoch) {
    return convertOffsetAndEpoch(in_time._unixUTCTimestamp, 0,
                                 epochStart::unix_epoch, out_utcOffsetSeconds,
                                 out_epoch);
}

time_t TimeUtils::getTimeT(epochTime in_time) {
    return static_cast<time_t>(
        getTimestamp(in_time, 0, TimeUtils::getCoreEpochStart()));
}

time_t TimeUtils::getTimeT(timestamp_t in_timestamp,
                           int32_t in_utcOffsetSeconds, epochStart in_epoch) {
    return static_cast<time_t>(TimeUtils::convertOffsetAndEpoch(
        in_timestamp, in_utcOffsetSeconds, in_epoch, 0,
        TimeUtils::getCoreEpochStart()));
}

epochTime TimeUtils::tmToEpochTime(tm timeParts) {
    _ensureInitialized();
#ifdef __TM_GMTOFF
    // Preserve the input offset as formatting metadata, but do not let mktime
    // use it to adjust or disambiguate the compiled timestamp.  LONG_MAX is not
    // a valid UTC offset, so implementations that optionally inspect this field
    // cannot treat it as an offset for the input calendar fields.
    const int32_t inputUtcOffsetSeconds =
        static_cast<int32_t>(timeParts.__TM_GMTOFF);
    timeParts.__TM_GMTOFF = LONG_MAX;
#endif
    // Convert the standard calendar fields using the processor's local
    // timezone. The saved __TM_GMTOFF value is not part of this conversion.
    time_t t = mktime(&timeParts);
    // cast into a timestamp_t
    timestamp_t ts = static_cast<timestamp_t>(t);

    // Create an epochTime object from the timestamp, knowing that the timestamp
    // we just got from mktime is using the core epoch and 0 offset
    epochTime result(ts, 0, TimeUtils::_core_epoch);
#ifdef __TM_GMTOFF
    // Store the original UTC offset only for later formatting.
    result._utcOffsetSeconds = inputUtcOffsetSeconds;
#endif
    // NOTE: We **intentionally** do not keep the
    // TimeUtils::_coreUtcOffsetSeconds as the _utcOffsetSeconds of the
    // epochTime object.
    return result;
}

void TimeUtils::fillTimeParts(epochTime in_time, tm& timeParts) {
    _ensureInitialized();
    // Get a timestamp in the processor's epoch and UTC timezone
    timestamp_t coreTime = TimeUtils::convertEpoch(
        static_cast<timestamp_t>(in_time._unixUTCTimestamp),
        epochStart::unix_epoch, TimeUtils::getCoreEpochStart());
    // cast back from timestamp_t to time_t; gmtime requires a true time_t (in
    // whatever size the core uses) as input!
    time_t gmtimeTime = static_cast<time_t>(coreTime);
    // convert the time stamp pointed to by gmtimeTime into broken-down time,
    // without doing any time-zone conversion.  The gmtime_r function treats the
    // input as UTC and gives an output in UTC.
    gmtime_r(&gmtimeTime, &timeParts);

#ifdef __TM_GMTOFF
    // A UTC tm should report UTC.
    timeParts.__TM_GMTOFF = 0;
#endif
}

void TimeUtils::fillTimeParts(timestamp_t in_timestamp,
                              int32_t in_utcOffsetSeconds, epochStart in_epoch,
                              tm& timeParts) {
    _ensureInitialized();
    // Convert the input timestamp to the processor's epoch and UTC timezone
    timestamp_t coreTime = TimeUtils::convertOffsetAndEpoch(
        in_timestamp, in_utcOffsetSeconds, in_epoch, 0,
        TimeUtils::getCoreEpochStart());
    // cast back from timestamp_t to time_t; gmtime requires a true time_t (in
    // whatever size the core uses) as input!
    time_t gmtimeTime = static_cast<time_t>(coreTime);
    // convert the time stamp pointed to by gmtimeTime into broken-down time,
    // without doing any time-zone conversion.  The gmtime_r function treats the
    // input as UTC and gives an output in UTC.
    gmtime_r(&gmtimeTime, &timeParts);

#ifdef __TM_GMTOFF
    timeParts.__TM_GMTOFF = 0;
#endif
}

bool TimeUtils::sameTime(const tm& a, const tm& b) {
    // make copies of the input tm structs so the originals are not modified by
    // mktime
    tm aCopy = a;
    tm bCopy = b;
#ifdef __TM_GMTOFF
    // UTC-offset fields are formatting metadata and must not affect the
    // timestamps compiled for comparison.
    aCopy.__TM_GMTOFF = LONG_MAX;
    bCopy.__TM_GMTOFF = LONG_MAX;
#endif
    // compare the compiled timestamps returned by mktime
    // NOTE: mktime() normalizes the given fields to valid ranges, applies the
    // local timezone (via tzset()), and uses tm_sec, tm_min, tm_hour, tm_mday,
    // tm_mon, tm_year, and tm_isdst to calculate the total seconds since the
    // epoch. The tm_wday and tm_yday values are overwritten with correct values
    // after computation; we copied the input tm structs to preserve the
    // originals.
    if (mktime(&aCopy) == static_cast<time_t>(-1) ||
        mktime(&bCopy) == static_cast<time_t>(-1)) {
        // If either timestamp could not be compiled, we cannot safely compare
        // them.
        return false;
    }
    return mktime(&aCopy) == mktime(&bCopy);
}

void TimeUtils::begin() {
    if (TimeUtils::_initialized) { return; }
    getProcessorEpochStart();  // Sets _core_epoch internally
    getProcessorTimeZone();    // Sets _coreUtcOffsetSeconds internally
    TimeUtils::_initialized = true;
}

void TimeUtils::_ensureInitialized() {
    if (!TimeUtils::_initialized) { TimeUtils::begin(); }
}

// Figure out when the epoch used by time_t starts for the processor.  The
// tm_year member of a tm struct is the number of years since 1900, per C/C++
// standard, but while time_t always contains a number of seconds since some
// epoch start, when the epoch starts, and the integer size of the time_t vary
// by Arduino cores and platforms.
epochStart TimeUtils::getProcessorEpochStart() {
    // Create a time_t representing the epoch start (0 seconds since the epoch)
    time_t epoch_zero = 0;
    // Use the core library to convert the time_t value into a tm struct.
    // The tm_year member is the number of years since 1900, per C/C++ standard.
    // The gmtime function converts a time_t value - in UTC - into a tm struct
    // representing UTC time.  The gmtime function does **not** do any timezone
    // conversions - it assumes the start and end point are in UTC.
    tm* epoch_zero_tm = gmtime(&epoch_zero);
    // If conversion failed, default to Unix epoch.
    if (epoch_zero_tm == nullptr) {
        TimeUtils::_core_epoch = epochStart::unix_epoch;
        return TimeUtils::_core_epoch;
    }
    // Since the tm_year member is the number of years since 1900, we add 1900
    // to get the actual year.
    const int zero_year = epoch_zero_tm->tm_year + 1900;
    switch (zero_year) {
        case 2000: TimeUtils::_core_epoch = epochStart::y2k_epoch; break;
        case 1980: TimeUtils::_core_epoch = epochStart::gps_epoch; break;
        case 1900: TimeUtils::_core_epoch = epochStart::nist_epoch; break;
        case 1970:
        default: TimeUtils::_core_epoch = epochStart::unix_epoch; break;
    }
    return TimeUtils::_core_epoch;
}

// This is yet another awkward function, but time support varies across device
// cores and I'm not sure if there is a better way to get the timezone offset
// that the processor/core considers "local time". mktime interprets its input
// fields as local time and returns the corresponding time_t in UTC.  That is,
// mktime **converts** the input to UTC based on the processor's local timezone.
// A conversionless version that assumes the input is already in UTC and returns
// UTC (ie, timegm(&timeParts) or mk_gmtime(const struct tm * timeptr)) is not
// available on all platforms.  Because we need to use mktime to compile time
// structs into timestamps, we need to know what conversion mktime is applying.
// I have no idea how to consistently set or detect the timezone across
// platforms, so instead we compare mktime's result for a known local calendar
// time to that instant in the detected core epoch.  Thankfully, we only have to
// do this once when we 'initialize.'
int32_t TimeUtils::getProcessorTimeZone() {
    // Create a time struct for Jan 1, 2000 at 00:00:00 in the processor's epoch
    tm timeParts;
    timeParts.tm_sec      = 0;
    timeParts.tm_min      = 0;
    timeParts.tm_hour     = 0;
    timeParts.tm_mday     = 1;
    timeParts.tm_mon      = 0;   /* tm_mon is 0-11 */
    timeParts.tm_year     = 100; /* tm_year is since 1900 */
    timeParts.tm_wday     = 0;   /* day of week, will be calculated */
    timeParts.tm_yday     = 0;   /* day of year, will be calculated */
    timeParts.tm_isdst    = 0;   /* daylight saving time flag */
    timestamp_t timeTimeT = mktime(&timeParts);

    // Check for mktime failure
    if (timeTimeT == (timestamp_t)-1) {
        TimeUtils::_coreUtcOffsetSeconds = 0;
        return 0;
    }

    // make a epoch time from the converted time
    // The getProcessorEpochStart() function must have already been called to
    // set the _core_epoch variable.
    // This function is protected to ensure the processor epoch has been
    // initialized before this function is called.
    epochTime timeEpoch(timeTimeT, 0, TimeUtils::_core_epoch);
    // get a timestamp in the Y2K epoch
    timestamp_t timeY2K = TimeUtils::getTimestamp(timeEpoch, 0,
                                                  epochStart::y2k_epoch);
    // Since we started with Jan 1, 2000, the offset from the input time and 0
    // in the Y2K epoch can only be caused by timezone shifts within the mktime
    // function.
    // Handle both signed and unsigned time_t properly
    // Check if time_t is signed by testing if (time_t)-1 < (time_t)0
    int32_t        coreUtcOffsetSeconds;
    constexpr bool is_time_t_signed = ((timestamp_t)-1 < (timestamp_t)0);

    if (is_time_t_signed) {
        // For signed time_t, negative values are represented normally
        if (timeY2K >= -static_cast<timestamp_t>(SECONDS_IN_DAY) &&
            timeY2K <= static_cast<timestamp_t>(SECONDS_IN_DAY)) {
            coreUtcOffsetSeconds = -static_cast<int32_t>(timeY2K);
        } else {
            coreUtcOffsetSeconds =
                0;  // Outside reasonable timezone range (±24 hours)
        }
    } else {
        // For unsigned time_t, check for wraparound indicating negative values
        if (timeY2K <= SECONDS_IN_DAY) {
            // A positive mktime delta represents a negative UTC offset.
            coreUtcOffsetSeconds = -static_cast<int32_t>(timeY2K);
        } else {
            // Check if this looks like a wrapped negative value
            const timestamp_t max_unsigned = (timestamp_t)-1;
            if (timeY2K > (max_unsigned - SECONDS_IN_DAY)) {
                // This is likely a wrapped negative offset
                timestamp_t utcOffsetMagnitudeSeconds = max_unsigned - timeY2K +
                    1;
                coreUtcOffsetSeconds =
                    static_cast<int32_t>(utcOffsetMagnitudeSeconds);
            } else {
                coreUtcOffsetSeconds = 0;  // Outside reasonable timezone range
            }
        }
    }
    TimeUtils::_coreUtcOffsetSeconds = coreUtcOffsetSeconds;
    return coreUtcOffsetSeconds;
}

// Test to see if a GPS second is a leap second.
bool TimeUtils::isLeap(timestamp_t gpsTime) {
    for (uint8_t i = 0; i < NUMBER_LEAP_SECONDS; ++i) {
        if (gpsTime == leapSeconds[i]) { return true; }
        if (gpsTime < leapSeconds[i]) { return false; }
    }
    return false;
}

// Count number of leap seconds that have passed.
int8_t TimeUtils::countLeaps(timestamp_t gpsTime, bool unix2gps) {
    int8_t nLeaps = 0;
    for (uint8_t i = 0; i < NUMBER_LEAP_SECONDS; ++i) {
        const timestamp_t leap = unix2gps ? leapSeconds[i] - i : leapSeconds[i];
        if (gpsTime < leap) { break; }
        ++nLeaps;
    }
    return nLeaps;
}

// Convert Unix Time to GPS Time.
timestamp_t TimeUtils::unix2gps(timestamp_t unixTime) {
    timestamp_t  gpsTime = unixTime - EPOCH_UNIX_TO_GPS;
    const int8_t nLeaps  = countLeaps(gpsTime, true);
    return gpsTime + nLeaps;
}

// Convert GPS Time to Unix Time.
timestamp_t TimeUtils::gps2unix(timestamp_t gpsTime) {
    timestamp_t  unixTime = gpsTime + EPOCH_UNIX_TO_GPS;
    const int8_t nLeaps   = countLeaps(gpsTime, false);
    return unixTime - nLeaps;
}

// cSpell:ignore
