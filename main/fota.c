#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_https_ota.h>
#include "stateProbe.h"
#include "trap.h"
#include "board_cfg.h"
#include "fota.h"

static const char TAG[] = "fota";
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

static const char fotaUpgradeCmd[] = "butler/upgrade";
static stateProbe_probe fotaUpgradeProbe;

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
  switch(evt->event_id) {
    case HTTP_EVENT_ERROR:
      ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
      break;
    case HTTP_EVENT_HEADER_SENT:
      ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
      break;
    case HTTP_EVENT_ON_HEADER:
      ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      break;
    case HTTP_EVENT_ON_FINISH:
      ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
      break;
    case HTTP_EVENT_DISCONNECTED:
      ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
      break;
    default:
      /* Handle unknown events gracefully */
      ESP_LOGD(TAG, "HTTP_EVENT_UNKNOWN: %d", evt->event_id);
  }
  return ESP_OK;
}

static void fota_upgrade(void* noArg)
{
  char url[BOARD_CFG_URL_SIZE] = {0};
  char furl[BOARD_CFG_URL_SIZE + 32] = {0};
  boardCfg_get_otaHost(url, BOARD_CFG_URL_SIZE);
  snprintf(furl, sizeof(furl), "https://%s:8070/%s", url, "butler.bin");

  esp_http_client_config_t config = {
    .url = furl,
    .cert_pem = (char *)server_cert_pem_start,
    .event_handler = _http_event_handler,
  };
#ifndef CONFIG_IDF_TARGET_ESP8266
  esp_https_ota_config_t ota_config = {
    .http_config = &config,
  };
#endif
  BUTLER_LOG("Firmware Upgrade Starting from: %s", furl);
#ifdef CONFIG_IDF_TARGET_ESP8266
  esp_err_t ret = esp_https_ota(&config);
#else
  esp_err_t ret = esp_https_ota(&ota_config);
#endif
  if (ret == ESP_OK) {
    BUTLER_LOG("Firmware Upgrade Successful. Restarting...");
    esp_restart();
  }
  else {
    BUTLER_LOG("Firmware Upgrade Failed");
  }
}

/*Call back upon receiving MQTT request from user to start fota*/
static void fota_upgrade_cb(const char* data, int length)
{
  xTaskCreate(&fota_upgrade, "fota", 4096, NULL, 5, NULL);
}

void fota_init(void)
{
  stateProbe_register(&fotaUpgradeProbe, fotaUpgradeCmd, fota_upgrade_cb);
}
