/**
 * @file Arduino.h
 * @copyright Stroud Water Research Center
 * Part of the EnviroDIY EpochTime library for Arduino.
 * This library is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Minimal Arduino String compatibility for native tests.
 */

#ifndef TESTS_NATIVE_ARDUINO_H_
#define TESTS_NATIVE_ARDUINO_H_

#include <stddef.h>
#include <stdint.h>

#include <string.h>

class String {
 public:
    String() : _value{} {}
    String(const char* value) : _value{} {
        if (value != nullptr) {
            strncpy(_value, value, sizeof(_value) - 1);
            _value[sizeof(_value) - 1] = '\0';
        }
    }

    const char* c_str() const {
        return _value;
    }

 private:
    char _value[64];
};

#endif  // TESTS_NATIVE_ARDUINO_H_
