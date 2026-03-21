#ifndef TIME_MOCK_H
#define TIME_MOCK_H

#include <time.h>
#include <stdbool.h>

// Mock interface for time functions
void time_mock_reset(void);
void time_mock_set_time(time_t mock_time);
time_t time_mock_get_time(void);
time_t time_mock_get_mktime_result(void);
void time_mock_set_mktime_result(time_t result);
bool time_mock_get_settimeofday_called(void);
void time_mock_set_settimeofday_result(int result);

#endif // TIME_MOCK_H