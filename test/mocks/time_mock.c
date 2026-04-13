#include "time_mock.h"
#include <time.h>
#include <sys/time.h>
#include <stdio.h>

static time_t mock_time = 0;
static time_t mock_mktime_result = 0;
static bool mock_settimeofday_called = false;
static int mock_settimeofday_result = 0;

void time_mock_reset(void)
{
    mock_time = 0;
    mock_mktime_result = 0;
    mock_settimeofday_called = false;
    mock_settimeofday_result = 0;
}

void time_mock_set_time(time_t t)
{
    mock_time = t;
}

time_t time_mock_get_time(void)
{
    return mock_time;
}

time_t time_mock_get_mktime_result(void)
{
    return mock_mktime_result;
}

void time_mock_set_mktime_result(time_t result)
{
    mock_mktime_result = result;
}

bool time_mock_get_settimeofday_called(void)
{
    return mock_settimeofday_called;
}

void time_mock_set_settimeofday_result(int result)
{
    mock_settimeofday_result = result;
}

// Mock implementations for time functions
time_t time(time_t *t)
{
    if (t) {
        *t = mock_time;
    }
    return mock_time;
}

time_t mktime(struct tm *tm)
{
    return mock_mktime_result;
}

int settimeofday(const struct timeval *tv, const struct timezone *tz)
{
    mock_settimeofday_called = true;
    if (tv) {
        mock_time = tv->tv_sec;
        printf("DEBUG: settimeofday called with tv_sec=%ld\n", (long)tv->tv_sec);
    }
    return mock_settimeofday_result;
}