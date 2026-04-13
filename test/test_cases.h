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
void test_nvCron_init_and_tick_single_minute_no_action(void);
void test_nvCron_trigger_on_at_exact_minute(void);
void test_nvCron_trigger_off_on_hour_change_and_day_change(void);
void test_nvCron_time_jump_more_than_one_minute_reinitialises(void);
void test_nvCron_multiple_ticks_same_minute_no_duplicate_trigger(void);
void test_nvCron_consecutive_minutes_triggers(void);
void test_nvCron_multiple_entries_same_minute_all_trigger(void);
void test_nvCron_hour_jump_more_than_one_hour_reinitialises(void);
void test_nvCron_before_and_after_hour_change_non_midnight(void);

// tmu tests
void test_tmu_set_valid_time(void);
void test_tmu_set_invalid_time(void);
void test_tmu_updateRTC_RTC_present(void);
void test_tmu_updateRTC_RTC_not_present(void);
void test_tmu_updateRTC_time_not_set(void);
void test_tmu_setup(void);
void test_tmu_teardown(void);

#endif // TEST_CASES_H