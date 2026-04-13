#include "unity.h"
#include "../mocks/nvs_mock.h"
#include "../mocks/freertos_mock.h"
#include "../mocks/actuator_mock.h"
#include "../../main/nvCron.h"
#include <string.h>

// Define CRON_MAX_ENTRIES from board_cfg.h
#define CRON_MAX_ENTRIES 15

// Define MIDNIGHT from nvCron.c
#define MIDNIGHT ((24<<8) | 0)

// Mock implementation of s_cronEntry from nvCron.c
typedef struct {
    uint16_t time;
    uint8_t func;
    uint8_t arg;
} s_cronEntry;

// Forward declarations for helper functions (used by tests defined earlier in this file)
static int entry_equal(const s_cronEntry* a, const s_cronEntry* b);
static int contains_entry(const s_cronEntry* arr, int count, const s_cronEntry* item);

// Test fixture setup and teardown
void test_nvCron_setup(void)
{
    // Reset mock state
    nvs_reset();
    freertos_mock_reset();
    actuator_mock_reset();
}

void test_nvCron_teardown(void)
{
    // No cleanup needed for mocks
}

// Test: nvCron_readMultipleEntry - empty list
void test_nvCron_readMultipleEntry_empty_list(void)
{
    // Arrange
    char buffer[CRON_MAX_ENTRIES * sizeof(s_cronEntry)] = {0};
    int length = sizeof(buffer);

    // Act
    int result = nvCron_readMultipleEntry(buffer, length);

    // Assert
    // nvCron_readMultipleEntry fills the provided buffer and returns the number of bytes written
    TEST_ASSERT_EQUAL(length, result);
    TEST_ASSERT_EQUAL(0, nvs_mock_get_read_count());
}

// Test: nvCron_readMultipleEntry - single entry
void test_nvCron_readMultipleEntry_single_entry(void)
{
    // Arrange
    s_cronEntry expected = {
        .time = 0x0800, // 08:00
        .func = CRON_FUNC_RELAY_ON,
        .arg = 1
    };

    char key[7];
    snprintf(key, sizeof(key), "cron%02d", 0);
    nvs_set_blob_simple(key, &expected, sizeof(expected));

    char buffer[CRON_MAX_ENTRIES * sizeof(s_cronEntry)] = {0};
    int length = sizeof(buffer);

    // Act
    int result = nvCron_readMultipleEntry(buffer, length);

    // Assert
    // nvCron_readMultipleEntry fills the provided buffer and returns bytes
    TEST_ASSERT_EQUAL(length, result);
    // The output may contain leading blank entries; check that the expected entry is present
    s_cronEntry* out = (s_cronEntry*)buffer;
    int out_count = result / sizeof(s_cronEntry);
    TEST_ASSERT_TRUE_MESSAGE(contains_entry(out, out_count, &expected), "Expected entry not found in output");
}

// Test: nvCron_readMultipleEntry - multiple entries
void test_nvCron_readMultipleEntry_multiple_entries(void)
{
    // Arrange
    s_cronEntry expected[3] = {
        { .time = 0x0800, .func = CRON_FUNC_RELAY_ON, .arg = 1 },
        { .time = 0x0C1E, .func = CRON_FUNC_RELAY_OFF, .arg = 2 },
        { .time = 0x1600, .func = CRON_FUNC_RELAY_ON, .arg = 3 }
    };

    for (int i = 0; i < 3; i++) {
        char key[7];
        snprintf(key, sizeof(key), "cron%02d", i);
    nvs_set_blob_simple(key, &expected[i], sizeof(expected[i]));
    }

    char buffer[CRON_MAX_ENTRIES * sizeof(s_cronEntry)] = {0};
    int length = sizeof(buffer);

    // Act
    int result = nvCron_readMultipleEntry(buffer, length);

    // nvCron_readMultipleEntry fills the provided buffer and returns bytes
    TEST_ASSERT_EQUAL(length, result);
    // The returned entries may not be in the same order. Verify each expected entry is present.
    s_cronEntry* out = (s_cronEntry*)buffer;
    int out_count = result / sizeof(s_cronEntry);
    for (int i = 0; i < 3; i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Expected entry %d not found in output", i);
        TEST_ASSERT_TRUE_MESSAGE(contains_entry(out, out_count, &expected[i]), msg);
    }
}

// Test: nvCron_readMultipleEntry - buffer too small
void test_nvCron_readMultipleEntry_buffer_too_small(void)
{
    // Arrange
    s_cronEntry expected[3] = {
        { .time = 0x0800, .func = CRON_FUNC_RELAY_ON, .arg = 1 },
        { .time = 0x0C1E, .func = CRON_FUNC_RELAY_OFF, .arg = 2 },
        { .time = 0x1600, .func = CRON_FUNC_RELAY_ON, .arg = 3 }
    };

    for (int i = 0; i < 3; i++) {
        char key[7];
        snprintf(key, sizeof(key), "cron%02d", i);
    nvs_set_blob_simple(key, &expected[i], sizeof(expected[i]));
    }

    char buffer[2 * sizeof(s_cronEntry)] = {0}; // Only space for 2
    int length = sizeof(buffer);

    // Act
    int result = nvCron_readMultipleEntry(buffer, length);

    // Assert (nvCron_readMultipleEntry returns bytes)
    TEST_ASSERT_EQUAL(2 * sizeof(s_cronEntry), result);
    // Since the buffer can only hold 2 entries, ensure exactly two of the expected three are present.
    s_cronEntry* out = (s_cronEntry*)buffer;
    int out_count = result / sizeof(s_cronEntry);
    int found = 0;
    for (int i = 0; i < 3; i++) {
        if (contains_entry(out, out_count, &expected[i])) found++;
    }
    TEST_ASSERT_EQUAL_MESSAGE(2, found, "Expected exactly 2 of the entries to be present in output");
}

// Helper: Serialize s_cronEntry into char buffer
static void serialize_entry(s_cronEntry* entry, char* buffer)
{
    memcpy(buffer, &entry->time, sizeof(entry->time));
    memcpy(buffer + sizeof(entry->time), &entry->func, sizeof(entry->func));
    memcpy(buffer + sizeof(entry->time) + sizeof(entry->func), &entry->arg, sizeof(entry->arg));
}

// Helper: compare two s_cronEntry
static int entry_equal(const s_cronEntry* a, const s_cronEntry* b)
{
    return (a->time == b->time) && (a->func == b->func) && (a->arg == b->arg);
}

// Helper: check if array contains an entry
static int contains_entry(const s_cronEntry* arr, int count, const s_cronEntry* item)
{
    for (int i = 0; i < count; i++) {
        if (entry_equal(&arr[i], item)) return 1;
    }
    return 0;
}

// Test: nvCron_writeMultipleEntry - single entry
void test_nvCron_writeMultipleEntry_single_entry(void)
{
    // Arrange
    nvs_reset();
    s_cronEntry entry = {
        .time = 0x0800,
        .func = CRON_FUNC_RELAY_ON,
        .arg = 1
    };
    char buffer[sizeof(s_cronEntry)] = {0};
    serialize_entry(&entry, buffer);

    // Act
    // Act
    printf("[test] buffer bytes:");
    for (size_t i=0;i<sizeof(buffer);i++) printf(" %02x", (unsigned char)buffer[i]);
    printf("\n");
    nvCron_writeMultipleEntry(buffer, sizeof(buffer));

    // Assert
    TEST_ASSERT_EQUAL(1, nvs_mock_get_write_count());
}

// Test: nvCron_writeMultipleEntry - multiple entries
void test_nvCron_writeMultipleEntry_multiple_entries(void)
{
    // Arrange
    nvs_reset();
    s_cronEntry entries[3] = {
        { .time = 0x0800, .func = CRON_FUNC_RELAY_ON, .arg = 1 },
        { .time = 0x0C1E, .func = CRON_FUNC_RELAY_OFF, .arg = 2 },
        { .time = 0x1600, .func = CRON_FUNC_RELAY_ON, .arg = 3 }
    };
    char buffer[3 * sizeof(s_cronEntry)] = {0};
    for (int i = 0; i < 3; i++) {
        serialize_entry(&entries[i], buffer + i * sizeof(s_cronEntry));
    }

    // Act
    printf("[test] buffer bytes:");
    for (size_t b=0;b<sizeof(buffer);b++) printf(" %02x", (unsigned char)buffer[b]);
    printf("\n");
    nvCron_writeMultipleEntry(buffer, sizeof(buffer));

    // Assert
    TEST_ASSERT_EQUAL(3, nvs_mock_get_write_count());
}

// Test: nvCron_writeMultipleEntry - zero entries
void test_nvCron_writeMultipleEntry_zero_entries(void)
{
    // Arrange
    // Act
    nvCron_writeMultipleEntry(NULL, 0);

    // Assert
    TEST_ASSERT_EQUAL(0, nvs_mock_get_write_count());
}

// Test: nvCron_writeMultipleEntry - exceed max entries
void test_nvCron_writeMultipleEntry_exceed_max(void)
{
    // Arrange
    nvs_reset();
    s_cronEntry entries[16] = {0}; // 16 > CRON_MAX_ENTRIES (15)
    char buffer[16 * sizeof(s_cronEntry)] = {0};
    for (int i = 0; i < 16; i++) {
        serialize_entry(&entries[i], buffer + i * sizeof(s_cronEntry));
    }

    // Act
    nvCron_writeMultipleEntry(buffer, sizeof(buffer));

    // Assert
    TEST_ASSERT_EQUAL(0, nvs_mock_get_write_count());
}

// New behavioural tests using the real nvCron implementation and actuator mock
void test_nvCron_init_and_tick_single_minute_no_action(void)
{
    // No entries in NVS; init at 10:00
    nvCron_init(10, 0);
    // Tick called again for same minute -> no change
    nvCron_tick(10, 0);
    TEST_ASSERT_EQUAL(0, actuator_mock_get_on_count());
    TEST_ASSERT_EQUAL(0, actuator_mock_get_off_count());
}

void test_nvCron_trigger_on_at_exact_minute(void)
{
    // Arrange: add an entry at 10:01 to turn actuator 1 ON
    s_cronEntry entry = { .time = (10<<8) | 1, .func = CRON_FUNC_RELAY_ON, .arg = 1 };
    char key[7];
    snprintf(key, sizeof(key), "cron%02d", 0);
    nvs_set_blob_simple(key, &entry, sizeof(entry));

    // Init at 10:00 and tick to 10:01
    nvCron_init(10, 0);
    nvCron_tick(10, 1);

    // Expect actuator_on to have been called once
    TEST_ASSERT_EQUAL(1, actuator_mock_get_on_count());
}

void test_nvCron_trigger_off_on_hour_change_and_day_change(void)
{
    // Arrange: two entries - one at 23:59 OFF, one at 00:00 ON
    s_cronEntry e1 = { .time = (23<<8) | 59, .func = CRON_FUNC_RELAY_OFF, .arg = 2 };
    s_cronEntry e2 = { .time = (0<<8) | 0,   .func = CRON_FUNC_RELAY_ON,  .arg = 2 };
    nvs_set_blob_simple("cron00", &e1, sizeof(e1));
    nvs_set_blob_simple("cron01", &e2, sizeof(e2));

    // Init at 23:58 then tick to 23:59 and then to 00:00
    nvCron_init(23, 58);
    nvCron_tick(23, 59);
    // OFF should have been called
    TEST_ASSERT_EQUAL(1, actuator_mock_get_off_count());
    nvCron_tick(0, 0);
    // ON should have been called
    TEST_ASSERT_EQUAL(1, actuator_mock_get_on_count());
}

void test_nvCron_time_jump_more_than_one_minute_reinitialises(void)
{
    // Arrange: an entry at 12:30 to turn on
    s_cronEntry entry = { .time = (12<<8) | 30, .func = CRON_FUNC_RELAY_ON, .arg = 3 };
    nvs_set_blob_simple("cron00", &entry, sizeof(entry));

    // Init at 12:00
    nvCron_init(12, 0);
    // Simulate jump to 12:35 (greater than 1 minute) - should reinit and not trigger missed 12:30
    nvCron_tick(12, 35);
    // No on call because we jumped past the event
    TEST_ASSERT_EQUAL(0, actuator_mock_get_on_count());
}

// New tests
void test_nvCron_multiple_ticks_same_minute_no_duplicate_trigger(void)
{
    // Arrange: entry at 14:10 -> ON actuator 1
    s_cronEntry entry = { .time = (14<<8) | 10, .func = CRON_FUNC_RELAY_ON, .arg = 1 };
    nvs_set_blob_simple("cron00", &entry, sizeof(entry));

    // Init at 14:09 -> tick to 14:10 twice
    nvCron_init(14, 9);
    nvCron_tick(14, 10);
    TEST_ASSERT_EQUAL(1, actuator_mock_get_on_count());
    // second call in same minute should not trigger again
    nvCron_tick(14, 10);
    TEST_ASSERT_EQUAL(1, actuator_mock_get_on_count());
}

void test_nvCron_consecutive_minutes_triggers(void)
{
    // Arrange: 15:05 -> ON actuator 1, 15:06 -> OFF actuator 1
    s_cronEntry e_on  = { .time = (15<<8) | 5,  .func = CRON_FUNC_RELAY_ON,  .arg = 1 };
    s_cronEntry e_off = { .time = (15<<8) | 6,  .func = CRON_FUNC_RELAY_OFF, .arg = 1 };
    nvs_set_blob_simple("cron00", &e_on, sizeof(e_on));
    nvs_set_blob_simple("cron01", &e_off, sizeof(e_off));

    nvCron_init(15, 4);
    nvCron_tick(15, 5);
    TEST_ASSERT_EQUAL(1, actuator_mock_get_on_count());
    nvCron_tick(15, 6);
    TEST_ASSERT_EQUAL(1, actuator_mock_get_off_count());
}

void test_nvCron_multiple_entries_same_minute_all_trigger(void)
{
    // Arrange: three entries at 16:20
    s_cronEntry e1 = { .time = (16<<8) | 20, .func = CRON_FUNC_RELAY_ON,  .arg = 1 };
    s_cronEntry e2 = { .time = (16<<8) | 20, .func = CRON_FUNC_RELAY_OFF, .arg = 2 };
    s_cronEntry e3 = { .time = (16<<8) | 20, .func = CRON_FUNC_RELAY_ON,  .arg = 3 };
    nvs_set_blob_simple("cron00", &e1, sizeof(e1));
    nvs_set_blob_simple("cron01", &e2, sizeof(e2));
    nvs_set_blob_simple("cron02", &e3, sizeof(e3));

    nvCron_init(16, 19);
    nvCron_tick(16, 20);
    TEST_ASSERT_EQUAL(2, actuator_mock_get_on_count());
    TEST_ASSERT_EQUAL(1, actuator_mock_get_off_count());
}

void test_nvCron_hour_jump_more_than_one_hour_reinitialises(void)
{
    // Arrange: entry at 08:30
    s_cronEntry entry = { .time = (8<<8) | 30, .func = CRON_FUNC_RELAY_ON, .arg = 4 };
    nvs_set_blob_simple("cron00", &entry, sizeof(entry));

    // Init at 06:00, jump to 08:30 (large jump) -> should not trigger missed event
    nvCron_init(6, 0);
    nvCron_tick(8, 30);
    TEST_ASSERT_EQUAL(0, actuator_mock_get_on_count());
}

void test_nvCron_before_and_after_hour_change_non_midnight(void)
{
    // Arrange: entry at 10:00 ON
    s_cronEntry entry = { .time = (10<<8) | 0, .func = CRON_FUNC_RELAY_ON, .arg = 5 };
    nvs_set_blob_simple("cron00", &entry, sizeof(entry));

    // Init at 9:59, tick to 10:00 then 10:01
    nvCron_init(9, 59);
    nvCron_tick(10, 0);
    TEST_ASSERT_EQUAL(1, actuator_mock_get_on_count());
    nvCron_tick(10, 1);
    // Should not trigger again
    TEST_ASSERT_EQUAL(1, actuator_mock_get_on_count());
}
