#include "tmu.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

// Mock state for tmu
static char mock_time_string[32] = {0};
static int mock_time_set_called = 0;
static int mock_update_rtc_called = 0;

void tmu_set(const char* buffer, int length)
{
    if (length >= sizeof(mock_time_string)) {
        length = sizeof(mock_time_string) - 1;
    }
    memcpy(mock_time_string, buffer, length);
    mock_time_string[length] = '\0';
    mock_time_set_called = 1;
    /* Mimic production behaviour: tmu_set triggers an RTC update */
    mock_update_rtc_called = 1;
}

void tmu_updateRTC(void)
{
    mock_update_rtc_called = 1;
}

// Getter functions for test assertions
int tmu_mock_get_time_set_called(void)
{
    return mock_time_set_called;
}

int tmu_mock_get_update_rtc_called(void)
{
    return mock_update_rtc_called;
}

void tmu_mock_reset(void)
{
    mock_time_set_called = 0;
    mock_update_rtc_called = 0;
    mock_time_string[0] = '\0';
}