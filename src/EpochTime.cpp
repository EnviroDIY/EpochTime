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


time_t epochTime::convertEpoch(time_t in_timestamp, epochStart in_epoch,
                               epochStart out_epoch) {
    switch (in_epoch) {
        case epochStart::unix_epoch: {
            switch (out_epoch) {
                case epochStart::y2k_epoch: {
                    return in_timestamp - EPOCH_UNIX_TO_Y2K;
                }
                case epochStart::gps_epoch: {
                    return epochTime::unix2gps(in_timestamp);
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
                    return epochTime::unix2gps(in_timestamp +
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
                    return epochTime::gps2unix(in_timestamp);
                }
                case epochStart::y2k_epoch: {
                    return epochTime::gps2unix(in_timestamp) -
                        EPOCH_UNIX_TO_Y2K;
                }
                case epochStart::nist_epoch: {
                    return epochTime::gps2unix(in_timestamp) +
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
                    return epochTime::unix2gps(in_timestamp -
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

time_t epochTime::convertTZOffset(time_t in_timestamp, int32_t in_utcOffset,
                                  int32_t out_utcOffset) {
    return in_timestamp + (out_utcOffset - in_utcOffset);
}

time_t epochTime::convertEpochAndTZOffset(time_t     in_timestamp,
                                          epochStart in_epoch,
                                          int32_t    in_utcOffset,
                                          epochStart out_epoch,
                                          int32_t    out_utcOffset) {
    return convertTZOffset(convertEpoch(in_timestamp, in_epoch, out_epoch),
                           in_utcOffset, out_utcOffset);
}

time_t epochTime::convertEpoch(epochTime in_time, epochStart out_epoch) {
    return convertEpoch(in_time._unixTimestamp, epochStart::unix_epoch,
                        out_epoch);
}

time_t epochTime::convertTZOffset(epochTime in_time, int32_t out_utcOffset) {
    return convertTZOffset(in_time._unixTimestamp, in_time._utcOffset,
                           out_utcOffset);
}

time_t epochTime::convertEpochAndTZOffset(epochTime  in_time,
                                          epochStart out_epoch,
                                          int32_t    out_utcOffset) {
    return convertEpochAndTZOffset(in_time._unixTimestamp,
                                   epochStart::unix_epoch, in_time._utcOffset,
                                   out_epoch, out_utcOffset);
}


// Convert Unix Time to GPS Time
time_t epochTime::unix2gps(time_t unixTime) {
    // Add offset in seconds
    bool isLeap;
    if (fmod(unixTime, 1) != 0) {
        unixTime = unixTime - 0.5;
        isLeap   = 1;
    } else {
        isLeap = 0;
    }
    time_t gpsTime = unixTime - EPOCH_UNIX_TO_GPS;
    int8_t nLeaps  = countLeaps(gpsTime, true);
    gpsTime        = gpsTime + nLeaps + isLeap;
    return gpsTime;
}

// Convert GPS Time to Unix Time
time_t epochTime::gps2unix(time_t gpsTime) {
    // Add offset in seconds
    time_t unixTime = gpsTime + EPOCH_UNIX_TO_GPS;
    int8_t nLeaps   = countLeaps(gpsTime, false);
    unixTime        = unixTime - nLeaps;
    if (isLeap(gpsTime)) { unixTime = unixTime + 0.5; }
    return unixTime;
}

// Test to see if a GPS second is a leap second
bool epochTime::isLeap(uint32_t gpsTime) {
    bool isLeap = false;
    for (int8_t i = 0; i < NUMBER_LEAP_SECONDS; i++) {
        if (gpsTime == leapSeconds[i]) { isLeap = true; }
    }
    return isLeap;
}

// Count number of leap seconds that have passed
int8_t epochTime::countLeaps(uint32_t gpsTime, bool unix2gps) {
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

// Initialize the array for the leap seconds - taken from the defines
const uint32_t epochTime::leapSeconds[NUMBER_LEAP_SECONDS] = LEAP_SECONDS;

// Initialize the processor epoch
epochStart TimeUtils::_core_epoch = epochStart::y2k_epoch;
// Initialize the processor timezone offset
int32_t TimeUtils::_core_tz = 0;
// Initialize the flag tracking initialization state
bool TimeUtils::_initialized = false;

// This converts an epoch time (seconds since a fixed epoch start) into a
// ISO8601 formatted string. Code modified from parts of the SparkFun RV-8803
// library
String TimeUtils::formatISO8601(time_t     epochSeconds,
                                int8_t     epochSecondsUTCOffset,
                                epochStart epoch) {
    return formatISO8601(epochTime(epochSeconds, epoch), epochSecondsUTCOffset);
}
String TimeUtils::formatISO8601(epochTime in_time,
                                int8_t    epochSecondsUTCOffset) {
    _ensureInitialized();
    // Use the conversion function to get a temporary variable for the epoch
    // time in the epoch used by the processor core (i.e., used by gmtime).
    time_t t = epochTime::convertEpoch(in_time, TimeUtils::_core_epoch);

    // create a temporary time struct
    // tm is a struct for time parts, defined in time.h
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
    int8_t quarterHours = epochSecondsUTCOffset * 4;
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
                               time_t epochSeconds, epochStart epoch) {
    formatDateTime(buffer, fmt, epochTime(epochSeconds, epoch));
}
void TimeUtils::formatDateTime(char* buffer, const char* fmt,
                               epochTime in_time) {
    _ensureInitialized();
    // Use the conversion function to get a temporary variable for the epoch
    // time in the epoch used by the processor core (i.e., used by gmtime).
    time_t t = epochTime::convertEpoch(in_time, TimeUtils::_core_epoch);

    // create a temporary time struct
    // tm is a struct for time parts, defined in time.h
    struct tm* tmp = gmtime(&t);

    // use strftime (from time.h) to format the time
    strftime(buffer, 20, fmt, tmp);
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

bool TimeUtils::isTimeSane(time_t ts, int8_t utcOffset, epochStart epoch) {
    return isTimeSane(epochTime(ts, epoch), utcOffset);
}
bool TimeUtils::isTimeSane(epochTime in_time, int8_t utcOffset) {
    _ensureInitialized();
    time_t epochSeconds = epochTime::convertEpoch(in_time,
                                                  epochStart::unix_epoch) -
        static_cast<time_t>(utcOffset * 3600);
    if (epochSeconds < EARLIEST_SANE_UNIX_TIMESTAMP ||
        epochSeconds > LATEST_SANE_UNIX_TIMESTAMP) {
        return false;
    } else {
        return true;
    }
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
    time_t     epoch_zero    = 0;
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
    time_t timeTimeT   = mktime(&timeParts);

    // Check for mktime failure
    if (timeTimeT == (time_t)-1) {
        TimeUtils::_core_tz = 0;
        return 0;
    }

    // make a epoch time from the converted time
    // NOTE: Re-run getProcessorEpochStart() instead of calling _core_epoch in
    // case the functions are called out of order and _core_epoch hasn't been
    // set yet.
    epochTime timeEpoch(timeTimeT, getProcessorEpochStart());
    // convert to Y2K epoch
    time_t timeY2K = epochTime::convertEpoch(timeEpoch, epochStart::y2k_epoch);
    // Since we started with Jan 1, 2000, the offset from the input time and 0
    // in the Y2K epoch can only be caused by timezone shifts within the mktime
    // function.
    // Handle both signed and unsigned time_t properly
    // Check if time_t is signed by testing if (time_t)-1 < (time_t)0
    int32_t        tz_offset;
    constexpr bool is_time_t_signed = ((time_t)-1 < (time_t)0);

    if (is_time_t_signed) {
        // For signed time_t, negative values are represented normally
        if (timeY2K >= -static_cast<time_t>(SECONDS_IN_DAY) &&
            timeY2K <= static_cast<time_t>(SECONDS_IN_DAY)) {
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
            const time_t max_unsigned = (time_t)-1;
            if (timeY2K > (max_unsigned - SECONDS_IN_DAY)) {
                // This is likely a wrapped negative offset
                time_t offsetMagnitude = max_unsigned - timeY2K + 1;
                tz_offset              = -static_cast<int32_t>(offsetMagnitude);
            } else {
                tz_offset = 0;  // Outside reasonable timezone range
            }
        }
    }
    TimeUtils::_core_tz = tz_offset;
    return tz_offset;
}

// cSpell:words
