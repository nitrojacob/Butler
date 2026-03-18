#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_https_ota.h>
#include "stateProbe.h"
#include "board_cfg.h"
#include "fota.h"

static const char TAG[] = "fota.c";
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
  }
  return ESP_OK;
}

static void fota_upgrade(void* noArg)
{
  char url[BOARD_CFG_URL_SIZE] = {0};
  boardCfg_get_otaHost(url, BOARD_CFG_URL_SIZE);
  esp_http_client_config_t config = {
    .url = url,
    .cert_pem = (char *)server_cert_pem_start,
    .event_handler = _http_event_handler,
  };
  ESP_LOGE(TAG, "Firmware Upgrade Starting...");
  esp_err_t ret = esp_https_ota(&config);
  if (ret == ESP_OK) {
    ESP_LOGE(TAG, "Firmware Upgrade Successful. Restarting...");
    esp_restart();
  }
  else {
    ESP_LOGE(TAG, "Firmware Upgrade Failed");
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
