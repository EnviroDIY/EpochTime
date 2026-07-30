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

// This converts an epoch time (seconds since a fixed epoch start) into a
// ISO8601 formatted string. Code modified from parts of the SparkFun RV-8803
// library
String TimeUtils::formatISO8601(etime_t epochSeconds, int8_t utcOffsetHours,
                                epochStart epoch) {
    return formatISO8601(epochTime(epochSeconds, utcOffsetHours, epoch),
                         utcOffsetHours);
}
String TimeUtils::formatISO8601(epochTime in_time, int8_t utcOffsetHours) {
    _ensureInitialized();
    // Get a single-value timestamp for the input epochTime object in the epoch
    // and timezone offset used by the processor core (i.e., used by gmtime).
    etime_t t_c_e = TimeUtils::getTimestamp(in_time, TimeUtils::_core_tz,
                                            TimeUtils::_core_epoch);
    // convert that time to the desired timezone offset for printing
    // gmtime knows nothing about timezones, so we convert before calling it.
    // The conversion is done in seconds, so we multiply the input hours by 3600
    // to get seconds.
    time_t t = TimeUtils::convertTZOffset(t_c_e, TimeUtils::_core_tz,
                                           utcOffsetHours * 3600);

    // create a temporary time struct
    // tm is a struct for time parts, defined in time.h
    // NOTE: gmtime requires a true time_t as input!
    struct tm* tmp = gmtime(&t);

    // create a temporary buffer to put the timestamp into
    static char
        time8601tz[20];  // Max of yyyy-mm-ddThh:mm:ss with \0 terminator
    // use strftime (from time.h) to format the time
    strftime(time8601tz, 20, "%Y-%m-%dT%H:%M:%S", tmp);

    // Correct the timezone format
    // NOTE: the %z format from strftime formats the timezone as +hhmm, but we
    // need +hh:mm
    char   isotz[8];
    int8_t quarterHours = utcOffsetHours * 4;
    char   plusMinus    = '+';
    if (quarterHours < 0) {
        plusMinus = '-';
        quarterHours *= -1;
    }
    uint16_t tz_mins = quarterHours * 15;
    uint8_t  tzh     = tz_mins / 60;
    uint8_t  tzm     = tz_mins % 60;
    snprintf(isotz, sizeof(isotz), "%c%02d:%02d", plusMinus, tzh, tzm);

    return String(time8601tz) + String(isotz);
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
    struct tm* tmp = gmtime(&t);

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
    switch (in_epoch) {
        case epochStart::unix_epoch: {
            switch (out_epoch) {
                case epochStart::y2k_epoch: {
                    return in_timestamp - EPOCH_UNIX_TO_Y2K;
                }
                case epochStart::gps_epoch: {
                    return TimeUtils::unix2gps(in_timestamp);
                }
                case epochStart::nist_epoch: {
                    return in_timestamp + EPOCH_NIST_TO_UNIX;
                }
                case epochStart::unix_epoch:
                default: {
                    return in_timestamp;
                }
            }
        }
        case epochStart::y2k_epoch: {
            switch (out_epoch) {
                case epochStart::unix_epoch: {
                    return in_timestamp + EPOCH_UNIX_TO_Y2K;
                }
                case epochStart::gps_epoch: {
                    return TimeUtils::unix2gps(in_timestamp +
                                               EPOCH_UNIX_TO_Y2K);
                }
                case epochStart::nist_epoch: {
                    return in_timestamp + EPOCH_NIST_TO_UNIX +
                        EPOCH_UNIX_TO_Y2K;
                }
                case epochStart::y2k_epoch:
                default: {
                    return in_timestamp;
                }
            }
        }
        case epochStart::gps_epoch: {
            switch (out_epoch) {
                case epochStart::unix_epoch: {
                    return TimeUtils::gps2unix(in_timestamp);
                }
                case epochStart::y2k_epoch: {
                    return TimeUtils::gps2unix(in_timestamp) -
                        EPOCH_UNIX_TO_Y2K;
                }
                case epochStart::nist_epoch: {
                    return TimeUtils::gps2unix(in_timestamp) +
                        EPOCH_NIST_TO_UNIX;
                }
                case epochStart::gps_epoch:
                default: {
                    return in_timestamp;
                }
            }
        }
        case epochStart::nist_epoch: {
            switch (out_epoch) {
                case epochStart::unix_epoch: {
                    return in_timestamp - EPOCH_NIST_TO_UNIX;
                }
                case epochStart::y2k_epoch: {
                    return in_timestamp - EPOCH_NIST_TO_UNIX -
                        EPOCH_UNIX_TO_Y2K;
                }
                case epochStart::gps_epoch: {
                    return TimeUtils::unix2gps(in_timestamp -
                                               EPOCH_NIST_TO_UNIX);
                }
                case epochStart::nist_epoch:
                default: {
                    return in_timestamp;
                }
            }
        }
        default: {
            return in_timestamp;
        }
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

void TimeUtils::begin() {
    if (TimeUtils::_initialized) { return; }
    getProcessorEpochStart();  // Sets _core_epoch internally
    getProcessorTimeZone();    // Sets _core_tz internally
    TimeUtils::_initialized = true;
}

void TimeUtils::_ensureInitialized() {
    if (!TimeUtils::_initialized) { TimeUtils::begin(); }
}

// figure out where the epoch starts for the processor
// This is awkward, but I'm struggling to find any documentation on
// what the year component input should be for mktime  - and I'm pretty sure
// it varies across processors.  If both gmtime and strftime are time.h for
// the processor then this should work regardless of how the year is
// represented within the tm structs.
epochStart TimeUtils::getProcessorEpochStart() {
    // NOTE: gmtime requires a true time_t as input!
    time_t    epoch_zero    = 0;
    struct tm* epoch_zero_tm = gmtime(&epoch_zero);
    // create a temporary buffer to put the timestamp into
    char epoch_zero_year[5];  // Max of yyyy with \0 terminator
    // use strftime (from time.h) to format the time
    strftime(epoch_zero_year, 5, "%Y", epoch_zero_tm);
    int zero_year = atoi(epoch_zero_year);

    epochStart ret_val;
    switch (zero_year) {
        default:
        case 1970: ret_val = epochStart::unix_epoch; break;
        case 2000: ret_val = epochStart::y2k_epoch; break;
        case 1980: ret_val = epochStart::gps_epoch; break;
        case 1900: ret_val = epochStart::nist_epoch; break;
    }
    TimeUtils::_core_epoch = ret_val;
    return ret_val;
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
                tz_offset              = -static_cast<int32_t>(offsetMagnitude);
            } else {
                tz_offset = 0;  // Outside reasonable timezone range
            }
        }
    }
    TimeUtils::_core_tz = tz_offset;
    return tz_offset;
}

// Test to see if a GPS second is a leap second
bool TimeUtils::isLeap(uint32_t gpsTime) {
    bool isLeap = false;
    for (int8_t i = 0; i < NUMBER_LEAP_SECONDS; i++) {
        if (gpsTime == leapSeconds[i]) { isLeap = true; }
    }
    return isLeap;
}

// Count number of leap seconds that have passed
int8_t TimeUtils::countLeaps(uint32_t gpsTime, bool unix2gps) {
    int8_t nLeaps = 0;  // number of leap seconds prior to gpsTime
    for (int8_t i = 0; i < NUMBER_LEAP_SECONDS; i++) {
        if (unix2gps) {
            if (gpsTime >= leapSeconds[i] - i) { nLeaps++; }
        } else {
            if (gpsTime >= leapSeconds[i]) { nLeaps++; }
        }
    }
    return nLeaps;
}

// Convert Unix Time to GPS Time
etime_t TimeUtils::unix2gps(etime_t unixTime) {
    // Add offset in seconds
    bool isLeap;
    if (fmod(unixTime, 1) != 0) {
        unixTime = unixTime - 0.5;
        isLeap   = 1;
    } else {
        isLeap = 0;
    }
    etime_t gpsTime = unixTime - EPOCH_UNIX_TO_GPS;
    int8_t nLeaps  = countLeaps(gpsTime, true);
    gpsTime        = gpsTime + nLeaps + isLeap;
    return gpsTime;
}

// Convert GPS Time to Unix Time
etime_t TimeUtils::gps2unix(etime_t gpsTime) {
    // Add offset in seconds
    etime_t unixTime = gpsTime + EPOCH_UNIX_TO_GPS;
    int8_t nLeaps   = countLeaps(gpsTime, false);
    unixTime        = unixTime - nLeaps;
    if (isLeap(gpsTime)) { unixTime = unixTime + 0.5; }
    return unixTime;
}

// cSpell:words
