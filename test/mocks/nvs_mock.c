/*
 * nvs_mock.c
 *
 * Provide a small in-memory NVS mock that exposes both the simple helper
 * functions used by existing tests (nvs_set_blob / nvs_get_blob simple forms)
 * and the esp-idf style API used by production code (nvs_open, nvs_set_blob, etc.).
 */

#include "nvs_mock.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "esp_err.h"

typedef struct {
    char key[64];
    void* data;
    size_t size;
} nvs_mock_entry_t;

static nvs_mock_entry_t mock_entries[256];
static int mock_entry_count = 0;
static int read_count = 0;
static int write_count = 0;

void nvs_reset(void)
{
    for (int i = 0; i < mock_entry_count; i++) {
        free(mock_entries[i].data);
    }
    mock_entry_count = 0;
    read_count = 0;
    write_count = 0;
}

/* Helper: find index for a key, or -1 */
static int find_key_index(const char* key)
{
    for (int i = 0; i < mock_entry_count; i++) {
        if (strcmp(mock_entries[i].key, key) == 0) return i;
    }
    return -1;
}

/* esp-idf style NVS functions */
esp_err_t nvs_open(const char* name, nvs_open_mode_t mode, nvs_handle_t* out_handle)
{
    (void)name; (void)mode;
    if (out_handle) *out_handle = (nvs_handle_t)1;
    return ESP_OK;
}

esp_err_t nvs_close(nvs_handle_t handle)
{
    (void)handle;
    return ESP_OK;
}

esp_err_t nvs_set_blob(nvs_handle_t handle, const char* key, const void* data, size_t size)
{
    (void)handle;
    printf("[nvs_mock] set_blob key=%s size=%zu\n", key, size);
    int idx = find_key_index(key);
    if (idx >= 0) {
        free(mock_entries[idx].data);
        mock_entries[idx].data = malloc(size);
        memcpy(mock_entries[idx].data, data, size);
        mock_entries[idx].size = size;
    } else {
        if (mock_entry_count >= (int)(sizeof(mock_entries)/sizeof(mock_entries[0]))) return ESP_ERR_NO_MEM;
        nvs_mock_entry_t* entry = &mock_entries[mock_entry_count++];
        strncpy(entry->key, key, sizeof(entry->key)-1);
        entry->key[sizeof(entry->key)-1] = '\0';
        entry->size = size;
        entry->data = malloc(size);
        if (data && size > 0) memcpy(entry->data, data, size);
    }
    write_count++;
    return ESP_OK;
}

esp_err_t nvs_get_blob(nvs_handle_t handle, const char* key, void* out_data, size_t* out_size)
{
    (void)handle;
    int idx = find_key_index(key);
    if (idx < 0) return ESP_ERR_NVS_NOT_FOUND;
    if (out_size == NULL) return ESP_ERR_INVALID_ARG;
    if (out_data && *out_size >= mock_entries[idx].size) {
        memcpy(out_data, mock_entries[idx].data, mock_entries[idx].size);
        *out_size = mock_entries[idx].size;
    } else {
        /* Caller asked for size only - update it */
        *out_size = mock_entries[idx].size;
    }
    read_count++;
    return ESP_OK;
}

esp_err_t nvs_erase_key(nvs_handle_t handle, const char* key)
{
    (void)handle;
    int idx = find_key_index(key);
    if (idx < 0) return ESP_ERR_NVS_NOT_FOUND;
    free(mock_entries[idx].data);
    /* compact the array */
    for (int i = idx; i < mock_entry_count-1; i++) mock_entries[i] = mock_entries[i+1];
    mock_entry_count--;
    return ESP_OK;
}

esp_err_t nvs_commit(nvs_handle_t handle)
{
    (void)handle;
    return ESP_OK;

}

/* Backwards-compatible simple helpers used by existing tests in this repo */
void nvs_set_blob_simple(const char* key, const void* data, size_t size)
{
    nvs_set_blob((nvs_handle_t)0, key, data, size);
}

bool nvs_get_blob_simple(const char* key, void* out_data, size_t* out_size)
{
    size_t s = (out_size)?*out_size:0;
    esp_err_t err = nvs_get_blob((nvs_handle_t)0, key, out_data, &s);
    if (out_size) *out_size = s;
    return (err == ESP_OK);
}

int nvs_mock_get_read_count(void)
{
    return read_count;
}

int nvs_mock_get_write_count(void)
{
    return write_count;
}

int nvs_mock_get_blob_count(void)
{
    return mock_entry_count;
}