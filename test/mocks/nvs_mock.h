#ifndef NVS_MOCK_H
#define NVS_MOCK_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <esp_err.h>
#include <nvs.h>

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

/* Simple helpers (existing tests) */
void nvs_reset(void);
void nvs_set_blob_simple(const char* key, const void* data, size_t size);
bool nvs_get_blob_simple(const char* key, void* out_data, size_t* out_size);

/* esp-idf style API used by production code (mocked here) */
esp_err_t nvs_open(const char* name, nvs_open_mode_t mode, nvs_handle_t* out_handle);
void nvs_close(nvs_handle_t handle);
esp_err_t nvs_set_blob(nvs_handle_t handle, const char* key, const void* data, size_t size);
esp_err_t nvs_get_blob(nvs_handle_t handle, const char* key, void* out_data, size_t* out_size);
esp_err_t nvs_erase_key(nvs_handle_t handle, const char* key);
esp_err_t nvs_commit(nvs_handle_t handle);

int nvs_mock_get_read_count(void);
int nvs_mock_get_write_count(void);
int nvs_mock_get_blob_count(void);

#endif // NVS_MOCK_H