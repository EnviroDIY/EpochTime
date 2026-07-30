/** ============================================================================
 * @example{lineno} DS3231.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief This example demonstrates getting a time from a DS3231 RTC and
 * pretty-printing it to the serial port.
 * ========================================================================== */

// Include the main header
#include <EpochTime.h>
// Include the Wire library for I2C communication
#include <Wire.h>
// Include the DS3231 RTC library
#include <Sodaq_DS3231.h>

// The Arduino setup function, which runs once at startup
void setup() {
    // Start the serial port
    Serial.begin(115200);
    while (!Serial) {
        // Wait for the serial port to connect. Needed for native USB
        delay(10);
    }

    // Start the I2C bus
    Wire.begin();
    // Start the RTC
    rtc.begin();
}

void loop() {
    // get the current date-time from the RTC as a custom DateTime object from
    // the RTC library
    DateTime now = rtc.now();
    // The Sodaq/EnviroDIY DS3231 library has a getEpoch() function that returns
    // the time as a uint32_t in the Unix epoch.
    uint32_t ts = now.getEpoch();
    // print the time in ISO8601 format, assuming an offset of -5 hours (Eastern
    // Standard Time)
    Serial.print("ISO 8601: ");
    Serial.println(TimeUtils::formatISO8601(ts, -5, epochStart::unix_epoch));
    // Now convert the timestamp to a smarter epochTime object assuming the RTC
    // is programmed in UTC
    epochTime myEpochTime(ts, 0, epochStart::unix_epoch);
    // print the time in ISO8601 format, but with the printed time in UTC-5
    // (EST), not the RTC's timezone
    Serial.print("ISO 8601 (UTC-5): ");
    Serial.println(TimeUtils::formatISO8601(myEpochTime, -5 * 3600));
}
