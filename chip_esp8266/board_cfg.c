#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>
#include "board_cfg.h"

static const char TAG[] = "board_cfg.c";

/* NVS namespace and keys */
static const char NVS_NS[] = "bcfg";
static const char KEY_MQTT[] = "bcfgMqttBroker";
static const char KEY_NTP[]  = "bcfgNtpServer";
static const char KEY_OTA[]  = "bcfgOtaHost";

/* Helper: read a string from NVS. Returns strlen of resulting string on success, or -1
 * if the key was not found or on parameter error.
 */
static int boardCfg_readKey(const char* key, char* dest, int len)
{
    nvs_handle_t handle = (nvs_handle_t)0;
    size_t size = BOARD_CFG_URL_SIZE;

    if(dest == NULL || len <= 0) return -1;

    ESP_ERROR_CHECK(nvs_open(NVS_NS, NVS_READWRITE, &handle));  /* ReadWrite is necessary as for blank devices, the namespace doesn't even exist */
    esp_err_t nvs_err  = nvs_get_blob(handle, key, dest, &size);
    if(nvs_err == ESP_ERR_NVS_NOT_FOUND) {
        strncpy(dest, BOARD_CFG_DEFAULT_URL, len);
    }
    nvs_close(handle);

    return (int)strnlen(dest, BOARD_CFG_URL_SIZE);
}

int boardCfg_get_mqttBroker(char* name, int len)
{
    return boardCfg_readKey(KEY_MQTT, name, len);
}

int boardCfg_get_ntpServer(char* name, int len)
{
    return boardCfg_readKey(KEY_NTP, name, len);
}

int boardCfg_get_otaHost(char* name, int len)
{
    return boardCfg_readKey(KEY_OTA, name, len);
}

/* protocomm-style write handler. Accepts a packed buffer of three
 * null-terminated strings in order: mqttBroker\0 ntpServer\0 otaHost\0.
 * Mirrors the other *_wr handlers in the project. Writes/erases keys
 * accordingly and returns ESP_OK on success (even if individual writes
 * logged errors), or an esp_err_t if NVS open fails.
 */
esp_err_t boardCfg_wr_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                             uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    nvs_handle_t handle = (nvs_handle_t)0;

    ESP_LOGI(TAG, "boardCfg_wr_handler called: inlen=%d", (int)inlen);
    if(inbuf == NULL || inlen <= 0) {
        return ESP_OK; /* nothing to do */
    }

    ESP_ERROR_CHECK(nvs_open(NVS_NS, NVS_READWRITE, &handle));
    int i;
    int cursor = 0;
    for(i =0; i<3; i++)
    {
        char buf[BOARD_CFG_URL_SIZE] = {0};
        size_t actualSize = strlen((char*)(&inbuf[cursor]));
        size_t size = actualSize;
        if (actualSize >= BOARD_CFG_URL_SIZE){
            ESP_LOGE(TAG, "URL %d too long: %d bytes", i, (int)actualSize);
            size = BOARD_CFG_URL_SIZE - 1;
        }
        strncpy(buf, (char*)(&inbuf[cursor]), size);
        cursor += actualSize + 1; /* +1 for the null terminator */
        switch (i){
            case 0:
                ESP_ERROR_CHECK(nvs_set_blob(handle, KEY_NTP, buf, BOARD_CFG_URL_SIZE));
                break;
            case 1:
                ESP_ERROR_CHECK(nvs_set_blob(handle, KEY_MQTT, buf, BOARD_CFG_URL_SIZE));
                break;
            case 2:
                ESP_ERROR_CHECK(nvs_set_blob(handle, KEY_OTA, buf, BOARD_CFG_URL_SIZE));
                break;
        }
    }
    
    ESP_ERROR_CHECK(nvs_commit(handle));
    nvs_close(handle);

    /* No response payload required. Match other handlers by returning ESP_OK */
    if(outlen) *outlen = 0;
    if(outbuf) *outbuf = NULL;
    return ESP_OK;
}
