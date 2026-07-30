/**
 * @file EpochTime.h
 * @copyright Stroud Water Research Center
 * Part of the EnviroDIY EpochTime library for Arduino.
 * This library is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Contains the epochTime class for handling time objects with different
 * epoch starts, and the TimeUtils class for formatting, validation, and core
 * processor time configuration.
 */

// Header Guards
#ifndef SRC_EPOCHTIME_H_
#define SRC_EPOCHTIME_H_

// Include other in-library and external dependencies
#include <Arduino.h>
#include <time.h>

#if !defined(EARLIEST_SANE_UNIX_TIMESTAMP) || defined(DOXYGEN)
/**
 * @def EARLIEST_SANE_UNIX_TIMESTAMP
 * @brief The earliest unix timestamp that can be considered sane.
 *
 * January 1, 2025
 */
#define EARLIEST_SANE_UNIX_TIMESTAMP 1735689600
#endif

#if !defined(LATEST_SANE_UNIX_TIMESTAMP) || defined(DOXYGEN)
/**
 * @def LATEST_SANE_UNIX_TIMESTAMP
 * @brief The latest unix timestamp that can be considered sane.
 *
 * January 1, 2035
 */
#define LATEST_SANE_UNIX_TIMESTAMP 2051222400
#endif
// Static assert to validate timestamp bounds relationship
static_assert(EARLIEST_SANE_UNIX_TIMESTAMP < LATEST_SANE_UNIX_TIMESTAMP,
              "EARLIEST_SANE_UNIX_TIMESTAMP must be less than "
              "LATEST_SANE_UNIX_TIMESTAMP");

#ifndef EPOCH_NIST_TO_UNIX
/**
 * @brief The difference in seconds between the epoch value returned by the NIST
 * Network Time Protocol and the Unix epoch
 *
 * The NIST Network Time Protocol (RFC-1305 and later versions) and Time
 * Protocol (RFC-868) both return a uint32_t which is the number of seconds from
 * January 1, 1900.  This is 2208988800 seconds behind the UNIX epoch.
 *
 * The NIST epoch will roll over and cease to work for processors with a
 * uint32_t time_t on February 7, 2036 6:28:15 AM.
 */
#define EPOCH_NIST_TO_UNIX 2208988800
#endif
#ifndef EPOCH_UNIX_TO_Y2K
/**
 * @brief The epoch starting Jan 1, 2000, as some RTC's and Arduinos do
 * (946684800s ahead of UNIX epoch)
 */
#define EPOCH_UNIX_TO_Y2K 946684800
#endif
#ifndef EPOCH_UNIX_TO_GPS
/**
 * @brief The GPS epoch starting Jan 5, 1980 (was 315964800s ahead of UNIX epoch
 * at founding, has drifted farther apart due to leap seconds)
 */
#define EPOCH_UNIX_TO_GPS 315964800
#endif
#ifndef NUMBER_LEAP_SECONDS
/**
 * @brief The number of announced leap seconds as of February 24, 2025
 */
#define NUMBER_LEAP_SECONDS 18
#endif
#ifndef LEAP_SECONDS
/**
 * @brief The GPS epoch equivalent for each of the announced leap seconds as of
 * February 24, 2025
 */
#define LEAP_SECONDS                                                     \
    {46828800,  78364801,  109900802, 173059203,  252028804,  315187205, \
     346723206, 393984007, 425520008, 457056009,  504489610,  551750411, \
     599184012, 820108813, 914803214, 1025136015, 1119744016, 1167264017}
#endif

/// @brief The number of seconds in a day
#define SECONDS_IN_DAY 86400L

#if !defined(etime_t) &&                                           \
    (((defined(ARDUINO_SAM_DUE) || defined(ARDUINO_NANO_ESP32)) && \
      !defined(PLATFORMIO)) ||                                     \
     defined(ARDUINO_ARCH_ARC32) || defined(ARDUINO_ARCH_PIC32))
// For some idiotic reason, the SAM and the ARC32 core use a long integer
// (uint16_t) instead of a long long (uint32_t) for time_t, so we need to use a
// 32-bit unsigned integer for timestamps
#define etime_t uint32_t
#elif !defined(etime_t)
// Use the time_t type for timestamps
#define etime_t time_t
#endif


/**
 * @brief Set the epoch start value.
 *
 * @warning 8-bit AVR - and several other = processor cores use a uint32_t for
 * time_t. Given the start of the first epoch here is 1900, this will roll over
 * and cease to work for processors with a uint32_t time_t on February 7, 2036
 * 6:28:15 AM
 *
 * @note: for AVR boards time_t is a typedef for uint32_t, defined in time.h
 * For SAMD time_t is a typedef for __int_least64_t _timeval.h implicit cast to
 * time_t
 */
enum class epochStart : etime_t {
    unix_epoch =
        EPOCH_NIST_TO_UNIX,  ///< Use a Unix epoch, starting Jan 1, 1970.
                             ///< This is the default for this library
    y2k_epoch = EPOCH_NIST_TO_UNIX +
        EPOCH_UNIX_TO_Y2K,  ///< Use an epoch starting Jan 1, 2000, as
                            ///< some RTC's and Arduinos do (946684800s
                            ///< ahead of UNIX epoch)
    gps_epoch = EPOCH_NIST_TO_UNIX +
        EPOCH_UNIX_TO_GPS,  ///< Use the GPS epoch starting Jan 5, 1980
                            ///< (was 315964800s ahead of UNIX epoch at
                            ///< founding, has drifted farther apart due
                            ///< to leap seconds)
    nist_epoch = 0  ///< Use the epoch starting Jan 1, 1900 as returned by
                    ///< the NIST Network Time Protocol (RFC-1305 and later
                    ///< versions) and Time Protocol (RFC-868) (2208988800
                    ///< behind the UNIX epoch)
};


/**
 * @brief A class for time objects that are aware of their epoch start.
 *
 * This class stores timestamps internally in Unix epoch and provides methods
 * for converting between different epoch types. The functions for converting
 * between GPS and Unix epoch - taking leap seconds into account - were
 * converted from the php functions available at
 * https://www.andrews.edu/~tzs/timeconv/timealgorithm.html. From that site:
 *
 * > [W]hile there was an offset of 315964800 seconds between Unix and GPS
 * > time when GPS time began, that offset changes each time there is a leap
 * > second. GPS time labels each second uniquely including leap seconds while
 * > Unix time does not, preferring to count a constant number of seconds a
 * > day including those containing leap seconds.
 *
 * @important UTC offsets in the class are **signed** 32-bit integers for the
 * number of *seconds* from UTC.  Use positive values for east of UTC and
 * negative values for west of UTC.  UTC-5 would be -18000, UTC+5 would be
 * 18000.
 */
class epochTime {
 public:
    friend class TimeUtils;

    /**
     * @brief Constructor, requires the timestamp and epoch start as input.
     *
     * @param timestamp A timestamp - in seconds since the start of the
     * given epoch.
     * @param epoch The start of the epoch for the timestamp; optional, defaults
     * to Unix epoch.
     * @param utcOffset The offset from UTC in seconds for the timestamp;
     * optional, defaults to 0.
     */
    epochTime(etime_t timestamp, int32_t utcOffset = 0,
              epochStart epoch = epochStart::unix_epoch);

    /**
     * @brief Deleted bool conversion operator
     *
     * The bool conversion operator allows an object of this class to be
     * converted into a boolean. Deleting this prevents anyone from calling
     * `if(epochTime)`
     *
     * @see
     * https://stackoverflow.com/questions/4600295/what-is-the-meaning-of-operator-bool-const
     */
    explicit operator bool() const = delete;

    /**
     * @brief Equality comparison operator
     *
     * @param a Another epochTime object
     * @return True if the objects represent the same time
     */
    bool operator==(epochTime a) const {
        return _unixUTCTimestamp == a._unixUTCTimestamp;
    }

    /**
     * @brief In-equality comparison operator
     *
     * @param a Another epochTime object
     * @return True if the objects represent different times
     */
    bool operator!=(epochTime a) const {
        return _unixUTCTimestamp != a._unixUTCTimestamp;
    }


    /**
     * @brief Get a single value timestamp from an epochTime object in a
     * specific epoch and timezone.
     *
     * @param out_utcOffset The desired UTC offset for the output, in seconds,
     * optional (defaults to 0).
     * @param out_epoch The desired epoch for the output, optional (defaults to
     * Unix epoch).
     * @return The timestamp in seconds since the start of the output epoch at
     * the output offset from UTC.
     * @note The out_utcOffset and out_epoch parameters are optional and default
     * to 0 and Unix epoch, respectively.
     */
    etime_t getTimestamp(int32_t    out_utcOffset = 0,
                         epochStart out_epoch     = epochStart::unix_epoch);

 private:
    /**
     * @brief Internal reference to the timestamp IN UNIX EPOCH
     */
    etime_t _unixUTCTimestamp;
};


/**
 * @brief Static utility class for time formatting, validation, and core
 * processor time configuration.
 *
 * @note This is a static-only class with all static functions and a deleted
 * constructor.
 *
 * @todo Support half/quarter hour time zones
 *
 * This class provides utilities for:
 * - Formatting timestamps into ISO8601 and custom string formats
 * - Validating timestamp sanity
 * - Abstracting processor/Arduino core epoch and timezone settings
 */
class TimeUtils {
 public:
    // Since all methods are static, disallow instantiation of this class.
    TimeUtils() = delete;

    /**
     * @brief Convert an epoch time (seconds since a fixed epoch start) into a
     * ISO8601 formatted string.
     *
     * Code modified from parts of the SparkFun RV-8803 library.
     *
     * @param epochSeconds The number of seconds since the start of the given
     * epoch.
     * @param utcOffsetHours The offset from UTC in **hours** for the printed
     * time.
     * @param epoch The epoch of the input epoch time.
     * @return An ISO8601 formatted String.
     *
     * @warning This function does **not** change the timezone of the input
     * epoch time. It assumes that the input timestamp value is already in the
     * correct timezone. The utcOffsetHours parameter is only used for
     * formatting the output string.
     */
    static String formatISO8601(etime_t epochSeconds, int8_t utcOffsetHours,
                                epochStart epoch);
    /**
     * @brief Convert an epochTime object into a ISO8601 formatted string.
     *
     * @param in_time An epochTime object
     * @param utcOffsetHours The offset from UTC in **hours** for the printed
     * time.
     * @return An ISO8601 formatted String.
     *
     * @remark This function assumes that the input epochTime object was created
     * with the correct UTC offset. It **will** convert the time to the given
     * utcOffsetHours for formatting the output string.
     */
    static String formatISO8601(epochTime in_time, int8_t utcOffsetHours);

    /**
     * @brief Convert a single value timestamp into a character string based on
     * the input strftime format string and put it into the given buffer.
     *
     * @attention This function DOES NOT SUPPORT TIMEZONES. Do not use the %z or
     * %Z inputs!
     *
     * @see https://en.cppreference.com/w/cpp/chrono/c/strftime for possible
     * formatting strings.
     *
     * @param buffer A buffer to put the finished string into. Make sure that
     * the buffer is big enough to hold all of the characters!
     * @param fmt The strftime format string.
     * @param epochSeconds The number of seconds since the start of the given
     * epoch in the given offset from UTC.
     * @param epoch The epoch of the input epoch time.
     *
     * @warning This function does NOT verify that the buffer is large enough to
     * hold the formatted string.  Make sure your buffer is large enough!
     */
    static void formatDateTime(char* buffer, const char* fmt,
                               etime_t epochSeconds, epochStart epoch);
    /**
     * @brief Convert a single value timestamp into a String object based on the
     * input strftime format string and put it into the given buffer.
     *
     * @attention This function DOES NOT SUPPORT TIMEZONES. Do not use the %z or
     * %Z inputs!
     *
     * @see https://en.cppreference.com/w/cpp/chrono/c/strftime for possible
     * formatting strings.
     *
     * @param fmt The strftime format string.
     * @param epochSeconds The number of seconds since the start of the given
     * epoch in the given offset from UTC.
     * @param epoch The epoch of the input epoch time.
     * @return A String object containing the formatted date and time.
     */
    static String formatDateTime(const char* fmt, etime_t epochSeconds,
                                 epochStart epoch);
    /**
     * @brief Convert an epoch time into a character string based on the input
     * strftime format string and put it into the given buffer.
     *
     * @note This function DOES NOT SUPPORT TIMEZONES. Do not use the %z or %Z
     * inputs!
     *
     * @see https://en.cppreference.com/w/cpp/chrono/c/strftime for possible
     * formatting strings.
     *
     * @param buffer A buffer to put the finished string into. Make sure that
     * the buffer is big enough to hold all of the characters!
     * @param fmt The strftime format string.
     * @param in_time An epochTime object
     *
     * @warning This function does NOT verify that the buffer is large enough to
     * hold the formatted string.  Make sure your buffer is large enough!
     */
    static void formatDateTime(char* buffer, const char* fmt,
                               epochTime in_time);
    /**
     * @brief Convert an epoch time into a String object based on the input
     * strftime format string and return it as a String object.
     *
     * @note This function DOES NOT SUPPORT TIMEZONES. Do not use the %z or %Z
     * inputs!
     *
     * @see https://en.cppreference.com/w/cpp/chrono/c/strftime for possible
     * formatting strings.
     *
     * @param fmt The strftime format string.
     * @param in_time An epochTime object
     * @return A String object containing the formatted date and time.
     */
    static String formatDateTime(const char* fmt, epochTime in_time);

    /**
     * @brief Gets a string name for the epoch     *
     * @param epoch The epoch to get the name of
     * @return The name for the epoch
     */
    static String printEpochName(epochStart epoch);
    /**
     * @brief Gets a string for the start date of the epoch
     * @param epoch The epoch to get the name of
     * @return The starting date, in ISO8601
     */
    static String printEpochStart(epochStart epoch);

    /**
     * @brief Check that a given epoch time (seconds since 1970) is within a
     * "sane" range.
     *
     * To be sane, the clock must be between #EARLIEST_SANE_UNIX_TIMESTAMP and
     * #LATEST_SANE_UNIX_TIMESTAMP.
     *
     * @param ts The timestamp to check (in seconds since the start of the given
     * epoch).
     * @param utcOffset The offset of the epoch time from UTC in hours.
     * @param epoch The type of epoch to use (i.e., the standard for the start
     * of the epoch).
     * @return True if the given time passes sanity range checking.
     */
    static bool isTimeSane(etime_t ts, int8_t utcOffset, epochStart epoch);
    /**
     * @brief Check that a given epoch time (an epochTime object) is within a
     * "sane" range.
     *
     * To be sane, the clock must be between #EARLIEST_SANE_UNIX_TIMESTAMP and
     * #LATEST_SANE_UNIX_TIMESTAMP.
     *
     * @param in_time An epochTime object
     * @return True if the given time passes sanity range checking.
     */
    static bool isTimeSane(epochTime in_time);

    /**
     * @brief Convert a timestamp from one epoch to another within the same UTC
     * offset.
     *
     * @param in_timestamp The input timestamp in seconds since the start of
     * the input epoch.
     * @param in_epoch The epoch of the input timestamp.
     * @param out_epoch The desired epoch for the output.
     * @return The timestamp in seconds since the start of the output epoch.
     */
    static etime_t convertEpoch(etime_t in_timestamp, epochStart in_epoch,
                                epochStart out_epoch);

    /**
     * @brief Convert a timestamp from one timezone to another within the same
     * epoch.
     *
     * @param in_timestamp The input timestamp in seconds since the start of
     * the input epoch.
     * @param in_utcOffset The UTC offset of the input timestamp, in seconds.
     * @param out_utcOffset The desired UTC offset for the output, in seconds.
     * @return The timestamp in seconds since the start of the output epoch.
     */
    static etime_t convertTZOffset(etime_t in_timestamp, int32_t in_utcOffset,
                                   int32_t out_utcOffset);

    /**
     * @brief Convert a timestamp from one epoch to another with different UTC
     * offsets.
     *
     * @param in_timestamp The input timestamp in seconds since the start of
     * the input epoch.
     * @param in_utcOffset The UTC offset of the input timestamp, in seconds.
     * @param in_epoch The epoch of the input timestamp.
     * @param out_utcOffset The desired UTC offset for the output, in seconds.
     * @param out_epoch The desired epoch for the output.
     * @return The timestamp in seconds since the start of the output epoch.
     */
    static etime_t convertOffsetAndEpoch(etime_t    in_timestamp,
                                         int32_t    in_utcOffset,
                                         epochStart in_epoch,
                                         int32_t    out_utcOffset,
                                         epochStart out_epoch);

    /**
     * @brief Get a single value timestamp from an epochTime object in a
     * specific epoch and timezone.
     *
     * @param in_time An epochTime object.
     * @param out_utcOffset The desired UTC offset for the output, in seconds,
     * optional (defaults to 0).
     * @param out_epoch The desired epoch for the output, optional (defaults to
     * Unix epoch).
     * @return The timestamp in seconds since the start of the output epoch at
     * the output offset from UTC.
     * @note The out_utcOffset and out_epoch parameters are optional and default
     * to 0 and Unix epoch, respectively.
     */
    static etime_t getTimestamp(epochTime in_time, int32_t out_utcOffset = 0,
                                epochStart out_epoch = epochStart::unix_epoch);

    /**
     * @brief Initialize the core time configuration.
     *
     * This function is optional. The core time configuration is automatically
     * initialized on the first call to any static function that requires it
     * (getCoreEpochStart(), getCoreTimeZone(), formatISO8601(),
     * formatDateTime(), or isTimeSane()). This method is provided for
     * explicit control over when initialization occurs, allowing you to avoid
     * any timing overhead on your preferred initialization call.
     */
    static void begin();

    /**
     * @brief Get the epoch start for the processor/Arduino core.
     *
     * Triggers lazy initialization on first call if begin() has not been
     * called.
     *
     * @return The epoch start used by the processor/Arduino core's time.h
     * library.
     */
    static epochStart getCoreEpochStart() {
        _ensureInitialized();
        return TimeUtils::_core_epoch;
    }

    /**
     * @brief Get the timezone offset for the processor/Arduino core.
     *
     * Triggers lazy initialization on first call if begin() has not been
     * called.
     *
     * @return The timezone offset in seconds from UTC used by the
     * processor/Arduino core's time.h library.
     */
    static int32_t getCoreTimeZone() {
        _ensureInitialized();
        return TimeUtils::_core_tz;
    }

 protected:
    /**
     * @brief Ensure that the processor epoch and timezone have been
     * initialized.
     *
     * This is an internal function called on first use of any static function
     * that requires processor epoch or timezone information.
     */
    static void _ensureInitialized();

    /**
     * @brief Detect the epoch start defined for the Arduino core used by the
     * processor.
     *
     * The real time clock libraries mostly document this, but the cores for the
     * various Arduino processors don't. The time.h file is not much more than a
     * stub. This function probes the core's gmtime() implementation to
     * determine the epoch.
     *
     * @return The start of the epoch used by the processor core.
     */
    static epochStart getProcessorEpochStart();

    /**
     * @brief Detect the timezone offset defined for the Arduino core used by
     * the processor.
     *
     * This function probes the core's mktime() implementation to determine the
     * timezone offset.
     *
     * @return The timezone offset in seconds from UTC.
     */
    static int32_t getProcessorTimeZone();

    /**
     * @brief The start of the epoch for the processor core's internal time.h
     * library.
     */
    static epochStart _core_epoch;

    /**
     * @brief The timezone used by the processor core's internal time.h library,
     * in seconds from UTC.
     */
    static int32_t _core_tz;

    /**
     * @brief Flag to track whether initialization has been performed.
     */
    static bool _initialized;

 private:
    /**
     * @brief Array of leap seconds as of February 24, 2025
     */
    static const uint32_t leapSeconds[NUMBER_LEAP_SECONDS];

    /**
     * @brief Test to see if a GPS second is a leap second
     *
     * @param gpsTime A timestamp in the GPS epoch
     * @return True if the time is a leap second
     */
    static bool isLeap(uint32_t gpsTime);

    /**
     * @brief Count number of leap seconds that have passed between the start of
     * the GPS epoch and the given time
     *
     * @param gpsTime A timestamp in the GPS epoch
     * @param unix2gps True if the input time is in Unix epoch, false if it is
     * GPS epoch
     * @return The number of leap seconds that have passed between the start of
     * the GPS epoch and the given time
     */
    static int8_t countLeaps(uint32_t gpsTime, bool unix2gps);

    /**
     * @brief Convert Unix time to GPS time.
     *
     * @param unixTime A timestamp in the Unix epoch.
     * @return The timestamp in the GPS epoch.
     */
    static etime_t unix2gps(etime_t unixTime);

    /**
     * @brief Convert GPS time to Unix time.
     *
     * @param gpsTime A timestamp in the GPS epoch.
     * @return The timestamp in the Unix epoch.
     */
    static etime_t gps2unix(etime_t gpsTime);
};

#endif

// cSpell:ignore nist hile
