#include "unity.h"
#include "../mocks/nvs_mock.h"
#include "../mocks/freertos_mock.h"
#include "../mocks/nvCron_mock.h"
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

// Test fixture setup and teardown
void test_nvCron_setup(void)
{
    // Reset mock state
    nvs_reset();
    freertos_mock_reset();
    nvCron_mock_reset();
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
    TEST_ASSERT_EQUAL(0, result);
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
    nvs_set_blob(key, &expected, sizeof(expected));

    char buffer[CRON_MAX_ENTRIES * sizeof(s_cronEntry)] = {0};
    int length = sizeof(buffer);

    // Act
    int result = nvCron_readMultipleEntry(buffer, length);

    // Assert
    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL_MEMORY(&expected, buffer, sizeof(s_cronEntry));
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
        nvs_set_blob(key, &expected[i], sizeof(expected[i]));
    }

    char buffer[CRON_MAX_ENTRIES * sizeof(s_cronEntry)] = {0};
    int length = sizeof(buffer);

    // Act
    int result = nvCron_readMultipleEntry(buffer, length);

    // Assert
    TEST_ASSERT_EQUAL(3, result);
    TEST_ASSERT_EQUAL_MEMORY(expected, buffer, 3 * sizeof(s_cronEntry));
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
        nvs_set_blob(key, &expected[i], sizeof(expected[i]));
    }

    char buffer[2 * sizeof(s_cronEntry)] = {0}; // Only space for 2
    int length = sizeof(buffer);

    // Act
    int result = nvCron_readMultipleEntry(buffer, length);

    // Assert
    TEST_ASSERT_EQUAL(2, result);
    TEST_ASSERT_EQUAL_MEMORY(expected, buffer, 2 * sizeof(s_cronEntry));
}

// Helper: Serialize s_cronEntry into char buffer
static void serialize_entry(s_cronEntry* entry, char* buffer)
{
    memcpy(buffer, &entry->time, sizeof(entry->time));
    memcpy(buffer + sizeof(entry->time), &entry->func, sizeof(entry->func));
    memcpy(buffer + sizeof(entry->time) + sizeof(entry->func), &entry->arg, sizeof(entry->arg));
}

// Test: nvCron_writeMultipleEntry - single entry
void test_nvCron_writeMultipleEntry_single_entry(void)
{
    // Arrange
    s_cronEntry entry = {
        .time = 0x0800,
        .func = CRON_FUNC_RELAY_ON,
        .arg = 1
    };
    char buffer[sizeof(s_cronEntry)] = {0};
    serialize_entry(&entry, buffer);

    // Act
    nvCron_writeMultipleEntry(buffer, sizeof(buffer));

    // Assert
    TEST_ASSERT_EQUAL(1, nvs_mock_get_write_count());
}

// Test: nvCron_writeMultipleEntry - multiple entries
void test_nvCron_writeMultipleEntry_multiple_entries(void)
{
    // Arrange
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
