/**
 * Provisioning manager. This module is responsible for provisioning the device with WiFi credentials and other configuration parameters.
 * All new butlers will boot to this mode. This mode is a standalone mode, where configurations can be done locally through the app.
 * The app can also configure the Wifi SSID and password to change it to a normal mode
 * 
 * Modes
 *                   Initial Boot --> Standalone (provisioning.c) --> Normal (main.c)
 *
 * All non-network functionality of the device should be available in the standalone mode, where configurations can be done locally through the app.
 */

#include <FreeRTOS.h>

#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_softap.h>
#include <esp_system.h>
#include <protocomm.h>
#include <esp_err.h>
#include <string.h>
#include <stdlib.h>

#include "heartBeat.h"
#include "stateProbe.h"
#include "nvCron.h"
#include "nvLogRing.h"
#include "tmu.h"
#include "trap.h"
#include "board_cfg.h"

#define COMM_STACK_SIZE 4096

#define NETWORK_CONNECTED 0x01
#define NETWORK_LOST      0x02
#define WIFI_DISCONNECTED 0x04

#define MIDNIGHT          ((24<<8) | 0)  /*MSB=24; LSB=00*/

// Define the cron entry structure (same as in nvCron.c)
typedef struct{
  U16 time;
  U8 func;
  U8 arg;
} s_cronEntry;

static const char TAG[] = "comm.c";
static os_timer_t wifiTimer;
static s_heartBeatHandle heartBeat;

/* States: MSB for Time, Then for WiFi, Then for IP
   St      | WiFi | IP | Time | Remarks
   111 (8) | NO   | NO | NO   | Powered On
   101 (6) | Yes  | NO | NO   |
   100 (5) | Yes  | Yes| NO   |
   011 (4) | NO   | NO | Yes  | Time read from RTC
   001 (2) | Yes  | NO | Yes  | Wifi Connected
   000 (1) | Yes  | Yes| Yes  | Wifi Connected, Got IP
   State +1 is used as the heartBeat flicker count
*/
static volatile char gState = 0x3;

static uint8_t cronBuf[CRON_MAX_ENTRIES * sizeof(s_cronEntry)] = {0};

// Custom endpoint handler for cron data exchange
static esp_err_t cron_rd_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                       uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    size_t size;

    ESP_LOGI(TAG, "Cron rd handler called with session_id: %d, inlen: %d", session_id, inlen);
    
    size = nvCron_readMultipleEntry((char*)cronBuf, sizeof(cronBuf));
    
    *outlen = size;
    *outbuf = cronBuf; // Since cronBuf is statically allocated, we can directly assign it to outbuf
    
    return ESP_OK;
}

static esp_err_t cron_wr_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                       uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{

    ESP_LOGI(TAG, "Cron wr handler called with session_id: %d, inlen: %d", session_id, inlen);
    
    if(inbuf != NULL && inlen > 0)
    {
        nvCron_writeMultipleEntry((const char*)inbuf, inlen);
    }
    
    *outlen = sizeof(s_cronEntry);
    *outbuf = cronBuf; // Since cronBuf is statically allocated, we can directly assign it to outbuf
    
    return ESP_OK;
}

static esp_err_t time_wr_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                       uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{

    ESP_LOGI(TAG, "Time wr handler called with session_id: %d, inlen: %d", session_id, inlen);
    
    if(inbuf != NULL && inlen > 0)
    {
        tmu_set((const char*)inbuf, inlen);
    }
    
    *outlen = sizeof(s_cronEntry);
    *outbuf = cronBuf; // Since cronBuf is statically allocated, we can directly assign it to outbuf
    
    return ESP_OK;
}

/* wifi_prov_mgr endpoint callback handler */
esp_err_t plog_rd_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                     uint8_t **outbuf, ssize_t *outlen, void *priv_data) {
    size_t size;
    ESP_LOGI(TAG, "Persitent Log Rd handler called with session_id: %d, inlen: %d", session_id, inlen);
    size = nvLogRing_read(cronBuf, sizeof(cronBuf));
    *outlen = size;
    *outbuf = (uint8_t*)cronBuf;

    return ESP_OK;
}

static void ip_cb(void* self, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if(event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
        gState &= 0xfe;
    }
    else if(event_id == IP_EVENT_STA_LOST_IP)
    {
        ESP_LOGI(TAG, "IP_EVENT_STA_LOST_IP");
        gState |=0x01;
    }
    heartBeat_reconfigure(&heartBeat, BOARD_CFG_HB_OFF_TIME, gState+1);
}

static void wifiTimerCb(void* unused)
{
    ESP_ERROR_CHECK(esp_wifi_connect());
}

static void wifi_cb(void* self, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "wifi_cb:%d", event_id);
    if(event_id == WIFI_EVENT_STA_START){
        ESP_ERROR_CHECK(esp_wifi_connect());
    }else if(event_id == WIFI_EVENT_STA_DISCONNECTED){
        os_timer_setfn(&wifiTimer, wifiTimerCb, NULL);
        os_timer_arm(&wifiTimer, BOARD_CFG_WIFI_RETRY_DELAY, 0);
        gState |= 0x2;
        heartBeat_reconfigure(&heartBeat, BOARD_CFG_HB_OFF_TIME, gState+1);
    }
    else if(event_id == WIFI_EVENT_STA_CONNECTED){
        gState &= 0xfd;
        heartBeat_reconfigure(&heartBeat, BOARD_CFG_HB_OFF_TIME, gState+1);
    }
}

static void prov_cb(void* self, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "Provisioning Host Disconnected");
    wifi_prov_mgr_stop_provisioning();
}

static void commPreInit(void * pvParameters)
{
    bool provisioned;
#ifdef CONFIG_RESET_PROVISIONED
    ESP_LOGE(TAG, "---------Resetting provisioned state----------");
    ESP_ERROR_CHECK(nvs_flash_deinit())
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
#endif /*CONFIG_RESET_PROVISIONED*/
    nvLogRing_init();
    tcpip_adapter_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Initialize WiFi provisioning manager with softap scheme
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_softap,
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
        .app_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
    };

    do{
        // Initialize provisioning manager
        ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
    
        // Create and register custom endpoint for cron data
        ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_create("cronRd"));
        ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_create("cronWr"));
        ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_create("timeWr"));
        ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_create("bcfgWr"));
        //ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_create(NVLOGRING_PROBE_ENDPOINT));

        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, WIFI_PROV_STA_DISCONNECTED, &prov_cb, NULL));
    
        
    
        // Check if device is already provisioned
        ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
        if (provisioned) {
            ESP_LOGI(TAG, "Device already provisioned. Skipping provisioning and starting normal WiFi mode.");
            break;
        }
        else {
            ESP_LOGI(TAG, "Starting provisioning...");
    
            // Get MAC address for unique service name and key
            uint8_t mac[6] = {0};
            esp_efuse_mac_get_default(mac);

            // Generate unique service name and key
            char service_name[] = "BUTLER_XXXXXX";
            char service_key[] = "XXXXXX_XXXXXX";
            char pop[] = "12345678";  // Default value if CONFIG_POP is not defined
            snprintf(service_name, sizeof(service_name), "BUTLER_%02x%02x%02x", mac[3], mac[4], mac[5]);
            snprintf(service_key, sizeof(service_key), "%02x%02x%02x_%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

            ESP_LOGI(TAG, "{\"ver\":\"v1\",\"name\":\"%s\",\"password\":\"%s\",\"pop\":\"%s\",\"transport\":\"softap\",\"security\":\"1\"}", service_name, service_key, pop);

            // Start provisioning with security version V1 (WIFI_PROV_SECURITY_1)
            ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1,
                                             pop,
                                             service_name,
                                             service_key));
            ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_register("cronRd", cron_rd_handler, NULL));
            ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_register("cronWr", cron_wr_handler, NULL));
            ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_register("timeWr", time_wr_handler, NULL));
            ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_register("bcfgWr", boardCfg_wr_handler, NULL));
            //ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_register(NVLOGRING_PROBE_ENDPOINT, plog_rd_handler, NULL));
            ESP_LOGI(TAG, "Heap after wifi_prov_mgr_endpoint_register: %d bytes", esp_get_free_heap_size());

            // Wait for provisioning to complete
            wifi_prov_mgr_wait();
        }
        ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

        wifi_prov_mgr_endpoint_unregister("cronRd");
        wifi_prov_mgr_endpoint_unregister("cronWr");
        wifi_prov_mgr_endpoint_unregister("timeWr");
        wifi_prov_mgr_endpoint_unregister("bcfgWr");
        //wifi_prov_mgr_endpoint_unregister(NVLOGRING_PROBE_ENDPOINT);
    

        // Deinitialize provisioning manager
        wifi_prov_mgr_deinit();
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_LOGI(TAG, "Heap after wifi_prov_mgr_deinit: %d bytes", esp_get_free_heap_size());
    }while(!provisioned);

    /* Connect to Wi-Fi */
    //ESP_ERROR_CHECK(wifi_prov_mgr_configure_sta(&wifi_cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_cb, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_cb, NULL));

    // Start Wi-Fi
    ESP_LOGI(TAG, "Starting Wi-Fi...");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    /*Kill*/
    vTaskDelete(NULL);
}

void commInit(void)
{
    heartBeat_init(&heartBeat, BOARD_CFG_HB_OFF_TIME, gState+1, INDICATOR_LED);
    xTaskCreate(commPreInit, "comm-pre", COMM_STACK_SIZE, NULL, 2, NULL);
}
