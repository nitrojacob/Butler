#include "nvCron.h"
#include <string.h>
#include <stdlib.h>

// Mock state for nvCron
static char mock_nvs_buffer[1024] = {0};
static int mock_nvs_buffer_size = 0;

void nvCron_mock_reset(void)
{
    memset(mock_nvs_buffer, 0, sizeof(mock_nvs_buffer));
    mock_nvs_buffer_size = 0;
}

int nvCron_readMultipleEntry(char* buffer, int length)
{
    int copy_len = (length < mock_nvs_buffer_size) ? length : mock_nvs_buffer_size;
    if (copy_len > 0) {
        memcpy(buffer, mock_nvs_buffer, copy_len);
    }
    return copy_len;
}

void nvCron_writeMultipleEntry(const char* buffer, int length)
{
    if (length > sizeof(mock_nvs_buffer)) {
        length = sizeof(mock_nvs_buffer);
    }
    if (length > 0 && buffer != NULL) {
        memcpy(mock_nvs_buffer, buffer, length);
        mock_nvs_buffer_size = length;
    } else {
        mock_nvs_buffer_size = 0;
    }
}