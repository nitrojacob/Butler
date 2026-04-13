#include "nvCron.h"
#include "nvs_mock.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef CRON_MAX_ENTRIES
#define CRON_MAX_ENTRIES 15
#endif

#ifndef MIDNIGHT
#define MIDNIGHT ((24<<8) | 0)
#endif

// Local copy of s_cronEntry used by tests/main implementation
typedef struct {
    uint16_t time;
    uint8_t func;
    uint8_t arg;
} s_cronEntry_local;

void nvCron_mock_reset(void)
{
    // Reset underlying NVS mock storage if available
    nvs_reset();
}

int nvCron_readMultipleEntry(char* buffer, int length)
{
    int i;
    size_t size = sizeof(s_cronEntry_local);
    int count = length / size;
    if (count > CRON_MAX_ENTRIES) count = CRON_MAX_ENTRIES;

    for (i = 0; i < count; i++) {
        char key[7];
        snprintf(key, sizeof(key), "cron%02d", i);
        size_t out_size = size;
        if (!nvs_get_blob(key, buffer + i * size, &out_size)) {
            // mark as invalid entry
            s_cronEntry_local blank = { .time = MIDNIGHT, .func = 0, .arg = 0 };
            memcpy(buffer + i * size, &blank, size);
        }
    }
    return count * size;
}

void nvCron_writeMultipleEntry(const char* buffer, int length)
{
    size_t size = sizeof(s_cronEntry_local);
    int count = length / size;
    if (count > CRON_MAX_ENTRIES) {
        // Do nothing if exceeds max entries to match original test expectation
        return;
    }
    for (int i = 0; i < count; i++) {
        char key[7];
        snprintf(key, sizeof(key), "cron%02d", i);
        nvs_set_blob(key, buffer + i * size, size);
    }
}