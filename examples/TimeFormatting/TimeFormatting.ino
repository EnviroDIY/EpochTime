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

#if (defined(ARDUINO_ARCH_NRF52840) || defined(ARDUINO_NRF52840_FEATHER)) && \
    !defined(PLATFORMIO)
#include <Adafruit_TinyUSB.h>  // for Serial
#endif

// set a value for the time
// Wednesday, July 1, 2026 at 12:00:00 AM GMT, in the Unix epoch
time_t     myTimestamp = 1782864000;
uint32_t   myOffset    = 0;  // UTC offset in seconds (0 for GMT)
epochStart myEpoch     = epochStart::unix_epoch;  // Use Unix epoch

// The Arduino setup function, which runs once at startup
void setup() {
    // Start the serial port
    Serial.begin(115200);
    while (!Serial) {
        // Wait for the serial port to connect. Needed for native USB
        delay(10);
    }

    Serial.println("epochTime and TimeUtils Demo");
    Serial.println("----------------------------");
    Serial.println("Input Timestamp: " +
                   String(static_cast<uint32_t>(myTimestamp)));
    Serial.println("Input Offset from UTC in Seconds: " + String(myOffset));
    Serial.println("Input Epoch: " + TimeUtils::printEpochName(myEpoch) +
                   " (start: " + TimeUtils::printEpochStart(myEpoch) + ")");

    // Print the core epoch and timezone
    Serial.print("Core epoch: ");
    Serial.println(TimeUtils::printEpochName(TimeUtils::getCoreEpochStart()));
    Serial.print("Core epoch start: ");
    Serial.println(TimeUtils::printEpochStart(TimeUtils::getCoreEpochStart()));
    Serial.print("Core timezone: ");
    Serial.println(TimeUtils::getCoreTimeZone());

    // Convert the timestamp to a timestamp with a different epoch and offset
    // convention
    // Convert to GMT-5 (Eastern Standard Time) without changing the epoch
    time_t tsEST = TimeUtils::convertTZOffset(myTimestamp, myOffset, -5 * 3600);
    Serial.print("Converted Timestamp (GMT-5): ");
    Serial.println(static_cast<uint32_t>(tsEST));
    // Convert to Y2K epoch without changing the timezone
    time_t tsY2K = TimeUtils::convertEpoch(myTimestamp, myEpoch,
                                           epochStart::y2k_epoch);
    Serial.print("Converted Timestamp (Y2K): ");
    Serial.println(static_cast<uint32_t>(tsY2K));
    // Change both the timezone and epoch to GMT-5 (Eastern Standard Time) in
    // the Y2K epoch
    time_t ts_EST_Y2K = TimeUtils::convertOffsetAndEpoch(
        myTimestamp, myOffset, myEpoch, -5 * 3600, epochStart::y2k_epoch);
    Serial.print("Converted Timestamp (GMT-5, Y2K): ");
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
    Serial.println("Checking if the converted timestamp is equal to the "
                   "epochTime object's timestamp: ");
    if (myEpochTime ==
        epochTime(ts_EST_Y2K, -5 * 3600, epochStart::y2k_epoch)) {
        Serial.println("\tYay! Conversion worked!");
    } else {
        Serial.println("\tWARNING: Conversion did NOT work!");
    }

    // check if the timestamp is equal to a different timestamp
    Serial.println("Checking that the epochTime object's timestamp is equal to "
                   "2026-07-30T01:13:46Z-04:00");
    if (myEpochTime ==
        epochTime(1785431629, -4 * 3600, epochStart::unix_epoch)) {
        Serial.println("\tThe timestamps are equal!");
    } else {
        Serial.println("\tThe timestamps are NOT equal!");
    }

    // Print the time in various formats
    Serial.println("\n\nTime Formatting Example");
    Serial.println("-----------------------");
    Serial.print("ISO 8601, UTC-5, from the epochTime object (timezone should "
                 "be converted): ");
    Serial.println(TimeUtils::formatISO8601(myEpochTime, -5));
    Serial.print("ISO 8601, input timezone, from a single timestamp (assumes "
                 "input and output are in the same timezone): ");
    Serial.println(TimeUtils::formatISO8601(myTimestamp, myOffset, myEpoch));
    Serial.print("Custom Format, from a epochTime object (timezone IGNORED): ");
    Serial.println(
        TimeUtils::formatDateTime("%A, %B %d, %Y %H:%M:%S", myEpochTime));
    Serial.print("Custom Format, from a single timestamp (timezone IGNORED): ");
    Serial.println(TimeUtils::formatDateTime("%A, %B %d, %Y %H:%M:%S",
                                             myTimestamp, myEpoch));

    // Print the time in various formats
    Serial.println("\n\nTime Sanity Checking Example");
    Serial.println("-----------------------");
    Serial.println("Is the timestamp sane? " +
                   String(TimeUtils::isTimeSane(myTimestamp, myOffset, myEpoch)
                              ? "Yes"
                              : "No"));
    Serial.println("Is the epochTime sane? " +
                   String(TimeUtils::isTimeSane(myEpochTime) ? "Yes" : "No"));
    Serial.println(
        "Is 2027-01-01T00:00:00Z sane? " +
        String(TimeUtils::isTimeSane(1893456000, 0, epochStart::unix_epoch)
                   ? "Yes"
                   : "No"));
    Serial.println(
        "Is 2023-01-01T00:00:00Z sane? " +
        String(TimeUtils::isTimeSane(1672531200, 0, epochStart::unix_epoch)
                   ? "Yes"
                   : "No"));
    Serial.println(
        "\tNOTE: January 1, 2023 is **not** considered to be a sane time "
        "because it is several years before the creation of this library.  If "
        "your Arduino real time clock returns this time, it is clearly not set "
        "correctly.");
}

// The Arduino loop function, which runs repeatedly after setup()
void loop() {
    // Nothing to do here
}
