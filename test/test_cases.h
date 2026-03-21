#ifndef TEST_CASES_H
#define TEST_CASES_H

// nvCron tests
void test_nvCron_readMultipleEntry_empty_list(void);
void test_nvCron_readMultipleEntry_single_entry(void);
void test_nvCron_readMultipleEntry_multiple_entries(void);
void test_nvCron_readMultipleEntry_buffer_too_small(void);
void test_nvCron_writeMultipleEntry_single_entry(void);
void test_nvCron_writeMultipleEntry_multiple_entries(void);
void test_nvCron_writeMultipleEntry_zero_entries(void);
void test_nvCron_writeMultipleEntry_exceed_max(void);
void test_nvCron_setup(void);
void test_nvCron_teardown(void);

// tmu tests
void test_tmu_set_valid_time(void);
void test_tmu_set_invalid_time(void);
void test_tmu_updateRTC_RTC_present(void);
void test_tmu_updateRTC_RTC_not_present(void);
void test_tmu_updateRTC_time_not_set(void);
void test_tmu_setup(void);
void test_tmu_teardown(void);

#endif // TEST_CASES_H