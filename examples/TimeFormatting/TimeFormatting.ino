/** ============================================================================
 * @example{lineno} TimeFormatting.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief This example demonstrates using the time formatting, conversion, and
 * sanity checking functions of the epochTime and TimeUtils classes
 * ========================================================================== */

// Include the main header
#include <EpochTime.h>

#if (defined(ARDUINO_NRF52840_FEATHER)) && !defined(ADAFRUIT_TINYUSB_H_)
#include <Adafruit_TinyUSB.h>  // for Serial
#endif

// set a value for the time
// Wednesday, July 1, 2026 at 12:00:00 AM GMT, in the Unix epoch
time_t     myTimestamp = 1782864000;
uint32_t   myOffset    = 0;  // UTC offset in seconds (0 for GMT)
epochStart myEpoch     = epochStart::unix_epoch;  // Use Unix epoch

String tmToArrayString(const tm& timeStruct) {
    String retStr;
    retStr.reserve(64);  // Reserve some space to avoid multiple reallocations
    retStr = "{" + String(timeStruct.tm_sec) + ", " +
        String(timeStruct.tm_min) + ", " + String(timeStruct.tm_hour) + ", " +
        String(timeStruct.tm_mday) + ", " + String(timeStruct.tm_mon) + ", " +
        String(timeStruct.tm_year) + ", " + String(timeStruct.tm_wday) + ", " +
        String(timeStruct.tm_yday) + ", " + String(timeStruct.tm_isdst);
#ifdef __TM_GMTOFF
    retStr += ", 0";
#endif
#ifdef __TM_ZONE
    retStr += ", \"GMT\"";
#endif
    retStr += "}";
    return retStr;
}

void printTmComponents(const tm& timeStruct, Stream& stream) {
    stream.print("    Year: ");
    stream.print(timeStruct.tm_year + 1900);
    stream.print(", Month: ");
    stream.print(timeStruct.tm_mon + 1);
    stream.print(", Day: ");
    stream.println(timeStruct.tm_mday);
    stream.print("    Day of Year: ");
    stream.print(timeStruct.tm_yday + 1);
    stream.print(", Day of Week: ");
    stream.println(timeStruct.tm_wday == 0 ? 7 : timeStruct.tm_wday);
    stream.print("    Hour: ");
    stream.print(timeStruct.tm_hour);
    stream.print(", Minute: ");
    stream.print(timeStruct.tm_min);
    stream.print(", Second: ");
    stream.println(timeStruct.tm_sec);
    stream.print("    DST Flag: ");
    stream.print(timeStruct.tm_isdst);
#ifdef __TM_GMTOFF
    stream.print("GMT Offset: ");
    stream.print(timeStruct.__TM_GMTOFF);
#endif
#ifdef __TM_ZONE
    stream.print("Time Zone: ");
    stream.print(timeStruct.__TM_ZONE);
#endif
    stream.println();
}

// The Arduino setup function, which runs once at startup
void setup() {
    // Start the serial port
    Serial.begin(115200);
    while (!Serial) {
        // Wait for the serial port to connect. Needed for native USB
        delay(10);
    }

    Serial.println(F("\n\n----------------------------"));
    Serial.println(F("epochTime and TimeUtils Demo"));
    Serial.println(F("----------------------------"));

    // Print the core epoch and timezone
    Serial.print(F("Core epoch: "));
    Serial.println(TimeUtils::printEpochName(TimeUtils::getCoreEpochStart()));
    Serial.print(F("Core epoch start: "));
    Serial.println(TimeUtils::printEpochStart(TimeUtils::getCoreEpochStart()));
    Serial.print(F("Core timezone: "));
    Serial.println(TimeUtils::getCoreTimeZone());

    Serial.println(F("\n\nEpoch Time and Unix Timestamp Conversion Example"));
    Serial.println(F("-----------------------"));

    Serial.print(F("Input Timestamp: "));
    Serial.println(static_cast<uint32_t>(myTimestamp));
    Serial.print(F("Input Offset from UTC in Seconds: "));
    Serial.println(myOffset);
    Serial.print("Input Epoch: ");
    Serial.print(TimeUtils::printEpochName(myEpoch));
    Serial.print(" (start: ");
    Serial.print(TimeUtils::printEpochStart(myEpoch));
    Serial.println(")");

    // Convert the timestamp to a timestamp with a different epoch and offset
    // convention
    // Convert to GMT-5 (Eastern Standard Time) without changing the epoch
    time_t tsEST = TimeUtils::convertTZOffset(myTimestamp, myOffset, -5 * 3600);
    Serial.print(F("Converted Timestamp (GMT-5): "));
    Serial.println(static_cast<uint32_t>(tsEST));
    // Convert to Y2K epoch without changing the timezone
    time_t tsY2K = TimeUtils::convertEpoch(myTimestamp, myEpoch,
                                           epochStart::y2k_epoch);
    Serial.print(F("Converted Timestamp (Y2K): "));
    Serial.println(static_cast<uint32_t>(tsY2K));
    // Change both the timezone and epoch to GMT-5 (Eastern Standard Time) in
    // the Y2K epoch
    time_t ts_EST_Y2K = TimeUtils::convertOffsetAndEpoch(
        myTimestamp, myOffset, myEpoch, -5 * 3600, epochStart::y2k_epoch);
    Serial.print(F("Converted Timestamp (GMT-5, Y2K): "));
    Serial.println(static_cast<uint32_t>(ts_EST_Y2K));

    // Create an epochTime object
    epochTime myEpochTime(myTimestamp, myOffset, myEpoch);
    // Get a single value timestamp in the Y2K epoch with a GMT-5 offset
    time_t ts_EST_Y2K_2 = TimeUtils::getTimestamp(myEpochTime, -5 * 3600,
                                                  epochStart::y2k_epoch);
    Serial.print(
        "Converted Timestamp (GMT-5, Y2K), from EpochTime static method: ");
    Serial.println(static_cast<uint32_t>(ts_EST_Y2K_2));
    // Another way to call the same function, using the epochTime object's
    // method
    time_t ts_EST_Y2K_3 = myEpochTime.getTimestamp(-5 * 3600,
                                                   epochStart::y2k_epoch);
    Serial.print(
        "Converted Timestamp (GMT-5, Y2K), from EpochTime object method: ");
    Serial.println(static_cast<uint32_t>(ts_EST_Y2K_3));

    // check if the timestamp is equal the converted value
    Serial.println(F("Checking if the converted timestamp is equal to the "
                     "epochTime object's timestamp: "));
    if (myEpochTime ==
        epochTime(ts_EST_Y2K, -5 * 3600, epochStart::y2k_epoch)) {
        Serial.println(F("\tYay! Conversion worked!"));
    } else {
        Serial.println(F("\tWARNING: Conversion did NOT work!"));
    }

    // check if the timestamp is equal to a different timestamp
    Serial.println(F("Checking that the epochTime object's timestamp is equal "
                     "to 2026-07-30T01:13:46Z-04:00"));
    if (myEpochTime ==
        epochTime(1785431629, -4 * 3600, epochStart::unix_epoch)) {
        Serial.println(F("\tThe timestamps are equal!"));
    } else {
        Serial.println(F("\tThe timestamps are NOT equal!"));
    }

    Serial.println(F("\n\ntm Comparison Example"));
    Serial.println(F("-----------------------"));


#ifdef __TM_GMTOFF
    Serial.println(F("This core supports __TM_GMTOFF"));
#endif
#ifdef __TM_ZONE
    Serial.println(F("This core supports __TM_ZONE"));
#endif

    tm myTM1       = {};
    myTM1.tm_sec   = 0;
    myTM1.tm_min   = 0;
    myTM1.tm_hour  = 0;  // 00:00:00
    myTM1.tm_mday  = 1;
    myTM1.tm_mon   = 7 - 1;  // July 1
    myTM1.tm_year  = 2026 - 1900;
    myTM1.tm_wday  = 0;  // ignored!
    myTM1.tm_yday  = 0;  // ignored!
    myTM1.tm_isdst = 0;  // ignored!
#ifdef __TM_GMTOFF
    myTM1.__TM_GMTOFF = 0;
#endif
#ifdef __TM_ZONE
    myTM1.__TM_ZONE = "GMT";
#endif

    tm myTM2       = {};
    myTM2.tm_sec   = 0;
    myTM2.tm_min   = 0;
    myTM2.tm_hour  = 19;  // 19:00:00
    myTM2.tm_mday  = 30;
    myTM2.tm_mon   = 6 - 1;  // June 30
    myTM2.tm_year  = 2026 - 1900;
    myTM2.tm_wday  = 0;  // ignored!
    myTM2.tm_yday  = 0;  // ignored!
    myTM2.tm_isdst = 0;  // ignored!
#ifdef __TM_GMTOFF
    myTM2.__TM_GMTOFF = -5 * 3600;
#endif
#ifdef __TM_ZONE
    myTM2.__TM_ZONE = "EST";
#endif

    // Tuesday, March 15, 2022 at 2:30:30 PM = 1647354630
    tm myTM3       = {};
    myTM3.tm_sec   = 30;
    myTM3.tm_min   = 30;
    myTM3.tm_hour  = 14;  // 14:30:30
    myTM3.tm_mday  = 15;
    myTM3.tm_mon   = 3 - 1;  // March 15
    myTM3.tm_year  = 2022 - 1900;
    myTM3.tm_wday  = 0;  // ignored!
    myTM3.tm_yday  = 0;  // ignored!
    myTM3.tm_isdst = 0;  // ignored!
#ifdef __TM_GMTOFF
    myTM3.__TM_GMTOFF = 0;
#endif
#ifdef __TM_ZONE
    myTM3.__TM_ZONE = "GMT";
#endif

    tm myTM4       = {};
    myTM4.tm_sec   = 0;
    myTM4.tm_min   = 0;
    myTM4.tm_hour  = 0;  // 00:00:00
    myTM4.tm_mday  = 1;
    myTM4.tm_mon   = 7 - 1;  // July 1
    myTM4.tm_year  = 2026 - 1900;
    myTM4.tm_wday  = 3;    // Wednesday
    myTM4.tm_yday  = 182;  // ignored!
    myTM4.tm_isdst = 0;    // ignored!
#ifdef __TM_GMTOFF
    myTM4.__TM_GMTOFF = 0;
#endif
#ifdef __TM_ZONE
    myTM4.__TM_ZONE = "GMT";
#endif

    Serial.print(F("The structs "));
    Serial.print(tmToArrayString(myTM1));
    Serial.print(F(" and "));
    Serial.print(tmToArrayString(myTM2));
    Serial.print(F(" are "));
    Serial.println(TimeUtils::sameTime(myTM1, myTM2) ? "equal" : "NOT equal");
#if defined(__TM_GMTOFF) || defined(__TM_ZONE)
    Serial.println(F("\tThey should be equal since the processing takes the "
                     "timezone into account."));
#else
    Serial.println(
        F("\tTimezone information is not available, equality may not "
          "account for timezone."));
#endif

    Serial.print(F("The structs "));
    Serial.print(tmToArrayString(myTM1));
    Serial.print(F(" and "));
    Serial.print(tmToArrayString(myTM3));
    Serial.print(F(" are "));
    Serial.println(TimeUtils::sameTime(myTM1, myTM3) ? F("equal")
                                                     : F("NOT equal"));
    Serial.println(
        F("\tThey should not be equal since they are different times"));

    Serial.print(F("The structs "));
    Serial.print(tmToArrayString(myTM1));
    Serial.print(F(" and "));
    Serial.print(tmToArrayString(myTM4));
    Serial.print(F(" are "));
    Serial.println(TimeUtils::sameTime(myTM1, myTM4) ? F("equal")
                                                     : F("NOT equal"));
    Serial.println(
        F("\tThey should be equal since wday and yday are ignored."));

    Serial.println(F("\n\ntm Conversion Example"));
    Serial.println(F("-----------------------"));

    epochTime compiledT1 = TimeUtils::tmToEpochTime(myTM1);
    Serial.print(tmToArrayString(myTM1));
    Serial.print(F(" compiled to "));
    Serial.println(static_cast<uint32_t>(compiledT1.getTimestamp()));

    epochTime compiledT4 = TimeUtils::tmToEpochTime(myTM4);
    Serial.print(tmToArrayString(myTM4));
    Serial.print(F(" compiled to "));
    Serial.println(static_cast<uint32_t>(compiledT4.getTimestamp()));

    epochTime compiledT2 = TimeUtils::tmToEpochTime(myTM2);
    Serial.print(tmToArrayString(myTM2));
    Serial.print(F(" compiled to "));
    Serial.println(static_cast<uint32_t>(compiledT2.getTimestamp()));

    epochTime compiledT3 = TimeUtils::tmToEpochTime(myTM3);
    Serial.print(tmToArrayString(myTM3));
    Serial.print(F(" compiled to "));
    Serial.println(static_cast<uint32_t>(compiledT3.getTimestamp()));


    Serial.println(F("\n\ntime_t Breakdown Example"));
    Serial.println(F("-----------------------"));

    // Convert a time_t object and into a tm structure
    tm     convertedTM   = {};
    time_t convertedTime = TimeUtils::getTimeT(myTimestamp, myOffset, myEpoch);
    TimeUtils::fillTimeParts(myTimestamp, myOffset, myEpoch, convertedTM);
    Serial.print(F("Timestamp: "));
    Serial.print(static_cast<uint32_t>(myTimestamp));
    Serial.print(F(" time_t: "));
    Serial.println(static_cast<uint32_t>(convertedTime));
    Serial.println(F("Timestamp broken into components: "));
    printTmComponents(convertedTM, Serial);

    tm convertedTM2 = {};
    // 1647354630 = Tuesday, March 15, 2022 at 2:30:30 PM
    time_t convertedTime2 = TimeUtils::getTimeT(1647354630, myOffset, myEpoch);
    TimeUtils::fillTimeParts(1647354630, myOffset, myEpoch, convertedTM2);
    Serial.print(F("Timestamp: "));
    Serial.print(static_cast<uint32_t>(1647354630));
    Serial.print(F(" time_t: "));
    Serial.println(static_cast<uint32_t>(convertedTime2));
    Serial.println(F("Timestamp broken into components: "));
    printTmComponents(convertedTM2, Serial);


    // Print the time in various formats
    Serial.println(F("\n\nTime Formatting Example"));
    Serial.println(F("-----------------------"));
    Serial.print(F("ISO 8601, UTC-5, from the epochTime object (timezone "
                   "should be converted): "));
    Serial.println(TimeUtils::formatISO8601(myEpochTime, -5));
    Serial.print(F("ISO 8601, input timezone, from a single timestamp (assumes "
                   "input and output are in the same timezone): "));
    Serial.println(TimeUtils::formatISO8601(myTimestamp, myOffset, myEpoch));
#if EPOCHTIME_ENABLE_STRFTIME
    Serial.print(
        F("Custom Format, from a epochTime object (timezone IGNORED): "));
    Serial.println(
        TimeUtils::formatDateTime("%A, %B %d, %Y %H:%M:%S", myEpochTime));
    Serial.print(
        F("Custom Format, from a single timestamp (timezone IGNORED): "));
    Serial.println(TimeUtils::formatDateTime("%A, %B %d, %Y %H:%M:%S",
                                             myTimestamp, myEpoch));
#endif

    // Print the time in various formats
    Serial.println(F("\n\nTime Sanity Checking Example"));
    Serial.println(F("-----------------------"));
    Serial.println(F("Is the timestamp sane? "));
    Serial.println(TimeUtils::isTimeSane(myTimestamp, myOffset, myEpoch)
                       ? F("Yes")
                       : F("No"));
    Serial.println(F("Is the epochTime sane? "));
    Serial.println(TimeUtils::isTimeSane(myEpochTime) ? F("Yes") : F("No"));
    Serial.println(F("Is 2027-01-01T00:00:00Z sane? "));
    Serial.println(TimeUtils::isTimeSane(1893456000, 0, epochStart::unix_epoch)
                       ? F("Yes")
                       : F("No"));
    Serial.println(F("Is 2023-01-01T00:00:00Z sane? "));
    Serial.println(TimeUtils::isTimeSane(1672531200, 0, epochStart::unix_epoch)
                       ? F("Yes")
                       : F("No"));
    Serial.println(
        F("\tNOTE: January 1, 2023 is **not** considered to be a sane time "
          "because it is several years before the creation of this library.  "
          "If your Arduino real time clock returns this time, it is clearly "
          "not set correctly."));
}

// The Arduino loop function, which runs repeatedly after setup()
void loop() {
    // Nothing to do here
}

// cSpell:ignore TINYUSB
