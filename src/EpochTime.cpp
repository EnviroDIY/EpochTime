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

// Initialize the array for the leap seconds - taken from the defines
const uint32_t TimeUtils::leapSeconds[NUMBER_LEAP_SECONDS] = LEAP_SECONDS;

// Initialize the processor epoch
epochStart TimeUtils::_core_epoch = epochStart::y2k_epoch;
// Initialize the processor timezone offset
int32_t TimeUtils::_core_tz = 0;
// Initialize the flag tracking initialization state
bool TimeUtils::_initialized = false;


epochTime::epochTime(etime_t timestamp, int32_t utcOffset, epochStart epoch) {
    _unixUTCTimestamp = TimeUtils::convertOffsetAndEpoch(
        timestamp, utcOffset, epoch, 0, epochStart::unix_epoch);
}

etime_t epochTime::getTimestamp(int32_t out_utcOffset, epochStart out_epoch) {
    return TimeUtils::getTimestamp(*this, out_utcOffset, out_epoch);
}

// Format an epoch time as ISO8601 without using strftime(), snprintf(), or
// intermediate String objects.  The caller must provide at least 26 bytes.
void TimeUtils::formatISO8601(char* buffer, epochTime in_time,
                              int8_t utcOffsetHours) {
    _ensureInitialized();

    // Convert to the processor core's epoch/timezone before using gmtime().
    etime_t t_core = TimeUtils::getTimestamp(in_time, TimeUtils::_core_tz,
                                             TimeUtils::_core_epoch);
    time_t  t      = static_cast<time_t>(TimeUtils::convertTZOffset(
        t_core, TimeUtils::_core_tz, utcOffsetHours * 3600L));

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
    int16_t offsetMinutes = static_cast<int16_t>(utcOffsetHours) * 60;
    char    sign          = '+';
    if (offsetMinutes < 0) {
        sign          = '-';
        offsetMinutes = -offsetMinutes;
    }
    const uint8_t offsetHours = static_cast<uint8_t>(offsetMinutes / 60);
    const uint8_t offsetMins  = static_cast<uint8_t>(offsetMinutes % 60);

    buffer[19] = sign;
    buffer[20] = static_cast<char>('0' + offsetHours / 10);
    buffer[21] = static_cast<char>('0' + offsetHours % 10);
    buffer[22] = ':';
    buffer[23] = static_cast<char>('0' + offsetMins / 10);
    buffer[24] = static_cast<char>('0' + offsetMins % 10);
    buffer[25] = '\0';
}

String TimeUtils::formatISO8601(etime_t epochSeconds, int8_t utcOffsetHours,
                                epochStart epoch) {
    return formatISO8601(epochTime(epochSeconds, utcOffsetHours, epoch),
                         utcOffsetHours);
}
String TimeUtils::formatISO8601(epochTime in_time, int8_t utcOffsetHours) {
    char buffer[26];
    formatISO8601(buffer, in_time, utcOffsetHours);
    return String(buffer);
}

void TimeUtils::formatDateTime(char* buffer, const char* fmt,
                               etime_t epochSeconds, epochStart epoch) {
    formatDateTime(buffer, fmt, epochTime(epochSeconds, 0, epoch));
}
String TimeUtils::formatDateTime(const char* fmt, etime_t epochSeconds,
                                 epochStart epoch) {
    return formatDateTime(fmt, epochTime(epochSeconds, 0, epoch));
}
void TimeUtils::formatDateTime(char* buffer, const char* fmt,
                               epochTime in_time) {
    _ensureInitialized();
    // Get a single-value timestamp for the input epochTime object in the epoch
    // used by the processor core (i.e., used by gmtime).
    time_t t = TimeUtils::getTimestamp(in_time, TimeUtils::_core_tz,
                                       TimeUtils::_core_epoch);

    // create a temporary time struct
    // tm is a struct for time parts, defined in time.h
    // NOTE: gmtime requires a true time_t as input!
    tm* tmp = gmtime(&t);

    // use strftime (from time.h) to format the time
    strftime(buffer, 39, fmt, tmp);
}
String TimeUtils::formatDateTime(const char* fmt, epochTime in_time) {
    // 38+1 for the longest common English format:
    // Wednesday, September 30, 2026 23:59:59
    char buffer[39];
    formatDateTime(buffer, fmt, in_time);
    return String(buffer);
}

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
        case epochStart::gps_epoch: return "1980-01-05T00:00:00Z";
        case epochStart::nist_epoch: return "1900-01-01T00:00:00Z";
        default: return "UNKNOWN";
    }
}

bool TimeUtils::isTimeSane(etime_t ts, int8_t utcOffset, epochStart epoch) {
    return isTimeSane(epochTime(ts, utcOffset, epoch));
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

etime_t TimeUtils::convertEpoch(etime_t in_timestamp, epochStart in_epoch,
                                epochStart out_epoch) {
    if (in_epoch == out_epoch) { return in_timestamp; }

    // Normalize the input to Unix epoch first, then convert Unix epoch to the
    // requested output epoch. This keeps the conversion matrix small while
    // preserving the GPS leap-second handling in unix2gps()/gps2unix().
    etime_t unixTimestamp;
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

etime_t TimeUtils::convertTZOffset(etime_t in_timestamp, int32_t in_utcOffset,
                                   int32_t out_utcOffset) {
    return in_timestamp + (out_utcOffset - in_utcOffset);
}

etime_t TimeUtils::convertOffsetAndEpoch(etime_t    in_timestamp,
                                         int32_t    in_utcOffset,
                                         epochStart in_epoch,
                                         int32_t    out_utcOffset,
                                         epochStart out_epoch) {
    return convertTZOffset(convertEpoch(in_timestamp, in_epoch, out_epoch),
                           in_utcOffset, out_utcOffset);
}

etime_t TimeUtils::getTimestamp(epochTime in_time, int32_t out_utcOffset,
                                epochStart out_epoch) {
    return convertOffsetAndEpoch(in_time._unixUTCTimestamp, 0,
                                 epochStart::unix_epoch, out_utcOffset,
                                 out_epoch);
}

time_t TimeUtils::getTimeT(epochTime in_time) {
    return static_cast<time_t>(getTimestamp(
        in_time, TimeUtils::getCoreTimeZone(), TimeUtils::getCoreEpochStart()));
}

time_t TimeUtils::getTimeT(etime_t in_timestamp, int32_t in_utcOffset,
                           epochStart in_epoch) {
    return static_cast<time_t>(TimeUtils::convertOffsetAndEpoch(
        in_timestamp, in_utcOffset, in_epoch, TimeUtils::getCoreTimeZone(),
        TimeUtils::getCoreEpochStart()));
}

time_t TimeUtils::tmToUTCTimeT(tm timeParts) {
    _ensureInitialized();
    // convert the time parts from the tm struct into an etime_t
    // the mktime function will return the time_t as seconds from 1/1/1970 in
    // the **processor's local time** converting from the timezone (if any)
    // given in the time parts to that processor local zone.
    etime_t t = static_cast<etime_t>(mktime(&timeParts));
    // Convert the etime_t (time_t) from the processor's timezone into UTC
    t = TimeUtils::convertOffsetAndEpoch(t, TimeUtils::getCoreTimeZone(),
                                         TimeUtils::getCoreEpochStart(), 0,
                                         TimeUtils::getCoreEpochStart());
    return static_cast<time_t>(t);
}

void TimeUtils::utcTimeTToTm(time_t t, tm& timeParts) {
    _ensureInitialized();
    // Convert the etime_t (time_t) from UTC to the processor's timezone
    etime_t coreTime = TimeUtils::convertOffsetAndEpoch(
        static_cast<etime_t>(t), 0, TimeUtils::getCoreEpochStart(),
        TimeUtils::getCoreTimeZone(), TimeUtils::getCoreEpochStart());
    // cast back from etime_t to time_t
    time_t gmtimeTime = static_cast<time_t>(coreTime);
    // converts the time stamp pointed to by gmtimeTime into broken-down time,
    // expressed as UTC
    gmtime_r(&gmtimeTime, &timeParts);
}

bool TimeUtils::sameTime(const tm& a, const tm& b) {
    return a.tm_sec == b.tm_sec && a.tm_min == b.tm_min &&
        a.tm_hour == b.tm_hour && a.tm_mday == b.tm_mday &&
        a.tm_mon == b.tm_mon && a.tm_year == b.tm_year;
}

void TimeUtils::begin() {
    if (TimeUtils::_initialized) { return; }
    getProcessorEpochStart();  // Sets _core_epoch internally
    getProcessorTimeZone();    // Sets _core_tz internally
    TimeUtils::_initialized = true;
}

void TimeUtils::_ensureInitialized() {
    if (!TimeUtils::_initialized) { TimeUtils::begin(); }
}

// Figure out when the epoch used by time_t starts for the processor.  The
// tm_year member of a tm struct is the number of years since 1900, per C/C++
// standard, but while time_t always contains a number of seconds since Some
// epoch start, when the epoch starts, and the integer size of the time_t vary
// by Arduino cores and platforms.
epochStart TimeUtils::getProcessorEpochStart() {
    // Create a time_t representing the epoch start (0 seconds since the epoch)
    time_t epoch_zero = 0;
    // Use the core library to convert the time_t value into a tm struct.
    // The tm_year member is the number of years since 1900, per C/C++ standard.
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
// that the processor/core considers "local time".  We need to know this because
// the mktime function converts the input time to the number of seconds since
// the epoch in the processor's timezone. The UTC version of the function
// (timegm(&timeParts)) is not available on all platforms, and I have no idea
// how to consistently set or detect the timezone across platforms, so instead
// we will just use mktime and then compare the returned timestamp to the known
// epoch start to figure out the offset.
int32_t TimeUtils::getProcessorTimeZone() {
    // Create a time struct for Jan 1, 2000 at 00:00:00 in the processor's epoch
    tm timeParts       = {};
    timeParts.tm_sec   = 0;
    timeParts.tm_min   = 0;
    timeParts.tm_hour  = 0;
    timeParts.tm_mday  = 1;
    timeParts.tm_mon   = 0;   /* tm_mon is 0-11 */
    timeParts.tm_year  = 100; /* tm_year is since 1900 */
    timeParts.tm_wday  = 0;   /* day of week, will be calculated */
    timeParts.tm_yday  = 0;   /* day of year, will be calculated */
    timeParts.tm_isdst = 0;   /* daylight saving time flag */
    etime_t timeTimeT  = mktime(&timeParts);

    // Check for mktime failure
    if (timeTimeT == (etime_t)-1) {
        TimeUtils::_core_tz = 0;
        return 0;
    }

    // make a epoch time from the converted time
    // The getProcessorEpochStart() function must have already been called to
    // set the _core_epoch variable.
    // This function is protected to ensure the processor epoch has been
    // initialized before this function is called.
    epochTime timeEpoch(timeTimeT, 0, TimeUtils::_core_epoch);
    // get a timestamp in the Y2K epoch
    etime_t timeY2K = TimeUtils::getTimestamp(timeEpoch, 0,
                                              epochStart::y2k_epoch);
    // Since we started with Jan 1, 2000, the offset from the input time and 0
    // in the Y2K epoch can only be caused by timezone shifts within the mktime
    // function.
    // Handle both signed and unsigned time_t properly
    // Check if time_t is signed by testing if (time_t)-1 < (time_t)0
    int32_t        tz_offset;
    constexpr bool is_time_t_signed = ((etime_t)-1 < (etime_t)0);

    if (is_time_t_signed) {
        // For signed time_t, negative values are represented normally
        if (timeY2K >= -static_cast<etime_t>(SECONDS_IN_DAY) &&
            timeY2K <= static_cast<etime_t>(SECONDS_IN_DAY)) {
            tz_offset = static_cast<int32_t>(timeY2K);
        } else {
            tz_offset = 0;  // Outside reasonable timezone range (±24 hours)
        }
    } else {
        // For unsigned time_t, check for wraparound indicating negative values
        if (timeY2K <= SECONDS_IN_DAY) {
            // Positive offset or zero
            tz_offset = static_cast<int32_t>(timeY2K);
        } else {
            // Check if this looks like a wrapped negative value
            const etime_t max_unsigned = (etime_t)-1;
            if (timeY2K > (max_unsigned - SECONDS_IN_DAY)) {
                // This is likely a wrapped negative offset
                etime_t offsetMagnitude = max_unsigned - timeY2K + 1;
                tz_offset = -static_cast<int32_t>(offsetMagnitude);
            } else {
                tz_offset = 0;  // Outside reasonable timezone range
            }
        }
    }
    TimeUtils::_core_tz = tz_offset;
    return tz_offset;
}

// Test to see if a GPS second is a leap second.
bool TimeUtils::isLeap(uint32_t gpsTime) {
    for (uint8_t i = 0; i < NUMBER_LEAP_SECONDS; ++i) {
        if (gpsTime == leapSeconds[i]) { return true; }
        if (gpsTime < leapSeconds[i]) { return false; }
    }
    return false;
}

// Count number of leap seconds that have passed.
int8_t TimeUtils::countLeaps(uint32_t gpsTime, bool unix2gps) {
    int8_t nLeaps = 0;
    for (uint8_t i = 0; i < NUMBER_LEAP_SECONDS; ++i) {
        const uint32_t leap = unix2gps ? leapSeconds[i] - i : leapSeconds[i];
        if (gpsTime < leap) { break; }
        ++nLeaps;
    }
    return nLeaps;
}

// Convert Unix Time to GPS Time.
etime_t TimeUtils::unix2gps(etime_t unixTime) {
    etime_t      gpsTime = unixTime - EPOCH_UNIX_TO_GPS;
    const int8_t nLeaps  = countLeaps(gpsTime, true);
    return gpsTime + nLeaps;
}

// Convert GPS Time to Unix Time.
etime_t TimeUtils::gps2unix(etime_t gpsTime) {
    etime_t      unixTime = gpsTime + EPOCH_UNIX_TO_GPS;
    const int8_t nLeaps   = countLeaps(gpsTime, false);
    return unixTime - nLeaps;
}

// cSpell:words
