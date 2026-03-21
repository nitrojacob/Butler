#include "unity.h"
#include "../mocks/time_mock.h"
#include "../mocks/ds1307_mock.h"
#include "../mocks/freertos_mock.h"
#include "../mocks/tmu_mock.h"
#include "../../main/tmu.h"
#include <time.h>
#include <string.h>

// Test fixture setup and teardown
void test_tmu_setup(void)
{
    // Reset all mocks before each test
    time_mock_reset();
    ds1307_mock_reset();
    tmu_mock_reset();
}

void test_tmu_teardown(void)
{
    // No cleanup needed for mocks
}

// Test: tmu_set - valid time string
void test_tmu_set_valid_time(void)
{
    // Arrange
    const char* time_str = "2026-03-07 12:30:00";
    int length = strlen(time_str);

    // Act
    tmu_set(time_str, length);

    // Assert
    TEST_ASSERT_EQUAL(1773004200, time_mock_get_time());
    TEST_ASSERT_EQUAL(1, time_mock_get_mktime_result());
    TEST_ASSERT_EQUAL(1, time_mock_get_settimeofday_called());
}

// Test: tmu_set - invalid time string
void test_tmu_set_invalid_time(void)
{
    // Arrange
    const char* time_str = "invalid_time_string";
    int length = strlen(time_str);

    // Act
    tmu_set(time_str, length);

    // Assert
    TEST_ASSERT_EQUAL(0, time_mock_get_time());
    TEST_ASSERT_EQUAL(0, time_mock_get_mktime_result());
    TEST_ASSERT_EQUAL(0, time_mock_get_settimeofday_called());
}

// Test: tmu_updateRTC - RTC present and time updated
void test_tmu_updateRTC_RTC_present(void)
{
    // Arrange
    ds1307_mock_set_present(true);
    time_mock_set_time(1773004200); // 2026-03-07 12:30:00 UTC

    // Act
    tmu_updateRTC();

    // Assert
    TEST_ASSERT_EQUAL(1773004200, ds1307_mock_get_time());
    TEST_ASSERT_EQUAL(1, ds1307_mock_get_set_time_count());
}

// Test: tmu_updateRTC - RTC not present
void test_tmu_updateRTC_RTC_not_present(void)
{
    // Arrange
    ds1307_mock_set_present(false);
    time_mock_set_time(1773004200); // 2026-03-07 12:30:00 UTC

    // Act
    tmu_updateRTC();

    // Assert
    TEST_ASSERT_EQUAL(0, ds1307_mock_get_time());
    TEST_ASSERT_EQUAL(0, ds1307_mock_get_set_time_count());
}

// Test: tmu_updateRTC - time not set yet
void test_tmu_updateRTC_time_not_set(void)
{
    // Arrange
    ds1307_mock_set_present(true);
    time_mock_set_time(0); // Time not set

    // Act
    tmu_updateRTC();

    // Assert
    TEST_ASSERT_EQUAL(0, ds1307_mock_get_time());
    TEST_ASSERT_EQUAL(0, ds1307_mock_get_set_time_count());
}