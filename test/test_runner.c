#include "unity.h"
#include "unity_config.h"
#include "mocks/nvCron_mock.h"
#include "mocks/tmu_mock.h"
#include "test_cases.h"

void setUp(void)
{
    // Call module-specific setup functions
    test_nvCron_setup();
    test_tmu_setup();
}

void tearDown(void)
{
    // Call module-specific teardown functions
    test_nvCron_teardown();
    test_tmu_teardown();
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_nvCron_readMultipleEntry_empty_list);
    RUN_TEST(test_nvCron_readMultipleEntry_single_entry);
    RUN_TEST(test_nvCron_readMultipleEntry_multiple_entries);
    RUN_TEST(test_nvCron_readMultipleEntry_buffer_too_small);
    RUN_TEST(test_nvCron_writeMultipleEntry_single_entry);
    RUN_TEST(test_nvCron_writeMultipleEntry_multiple_entries);
    RUN_TEST(test_nvCron_writeMultipleEntry_zero_entries);
    RUN_TEST(test_nvCron_writeMultipleEntry_exceed_max);

    RUN_TEST(test_tmu_set_valid_time);
    RUN_TEST(test_tmu_set_invalid_time);
    RUN_TEST(test_tmu_updateRTC_RTC_present);
    RUN_TEST(test_tmu_updateRTC_RTC_not_present);
    RUN_TEST(test_tmu_updateRTC_time_not_set);

    return UNITY_END();
}