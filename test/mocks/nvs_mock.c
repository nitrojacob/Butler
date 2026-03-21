#include "nvs_mock.h"
#include <stdlib.h>
#include <string.h>

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

void nvs_set_blob(const char* key, const void* data, size_t size)
{
    if (mock_entry_count >= 256) return;

    nvs_mock_entry_t* entry = &mock_entries[mock_entry_count++];
    strncpy(entry->key, key, sizeof(entry->key) - 1);
    entry->key[sizeof(entry->key) - 1] = '\0';
    entry->size = size;
    entry->data = malloc(size);
    if (data && size > 0) {
        memcpy(entry->data, data, size);
    }
    write_count++;
}

bool nvs_get_blob(const char* key, void* out_data, size_t* out_size)
{
    for (int i = 0; i < mock_entry_count; i++) {
        if (strcmp(mock_entries[i].key, key) == 0) {
            if (out_data && out_size && *out_size >= mock_entries[i].size) {
                memcpy(out_data, mock_entries[i].data, mock_entries[i].size);
                *out_size = mock_entries[i].size;
            }
            read_count++;
            return true;
        }
    }
    return false;
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