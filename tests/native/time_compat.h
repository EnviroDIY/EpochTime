#ifndef TESTS_NATIVE_TIME_COMPAT_H_
#define TESTS_NATIVE_TIME_COMPAT_H_

#include <time.h>
#include <cstring>

#ifdef _WIN32
/**
 * @brief Windows replacement for POSIX gmtime_r in native tests.
 *
 * @param timer Pointer to the time value to convert.
 * @param result Pointer to the tm structure that receives the UTC result.
 */
inline void epochTimeTestGmtimeR(const time_t* timer, tm* result) {
    gmtime_s(result, timer);
}
#define gmtime_r epochTimeTestGmtimeR
#endif

#endif  // TESTS_NATIVE_TIME_COMPAT_H_
