#ifndef TESTS_NATIVE_TIME_COMPAT_H_
#define TESTS_NATIVE_TIME_COMPAT_H_

#include <time.h>

#ifdef _WIN32
inline void epochTimeTestGmtimeR(const time_t* timer, tm* result) {
    gmtime_s(result, timer);
}
#define gmtime_r epochTimeTestGmtimeR
#endif

#endif  // TESTS_NATIVE_TIME_COMPAT_H_
