#ifndef NVS_MOCK_H
#define NVS_MOCK_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Forward declaration of nvCronEntry_t from main/nvCron.h
typedef struct {
    uint8_t minute;
    uint8_t hour;
    uint8_t dayOfMonth;
    uint8_t month;
    uint8_t dayOfWeek;
    char command[64];
    bool enabled;
} nvCronEntry_t;

// Mock interface for nvs functions
void nvs_reset(void);
void nvs_set_blob(const char* key, const void* data, size_t size);
bool nvs_get_blob(const char* key, void* out_data, size_t* out_size);
int nvs_mock_get_read_count(void);
int nvs_mock_get_write_count(void);
int nvs_mock_get_blob_count(void);

#endif // NVS_MOCK_H