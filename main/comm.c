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

#include <string.h>
#include <stdlib.h>

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#ifdef CONFIG_IDF_TARGET_ESP8266
    #include <wifi_provisioning/manager.h>
    #include <wifi_provisioning/scheme_softap.h>
#else
    #include <network_provisioning/manager.h>
    #include <network_provisioning/scheme_softap.h>
    #include <esp_netif.h>
#endif
#include <esp_system.h>
#include <protocomm.h>
#include <esp_err.h>

#include "heartBeat.h"
#include "stateProbe.h"
#include "nvCron.h"
#include "nvLogRing.h"
#include "tmu.h"
#include "trap.h"
#include "board_cfg.h"

#ifndef CONFIG_IDF_TARGET_ESP8266
    #define WIFI_PROV_EVENT                     NETWORK_PROV_EVENT
    #define WIFI_PROV_STA_DISCONNECTED          NETWORK_PROV_WIFI_STA_DISCONNECTED
    #define WIFI_PROV_SECURITY_1                NETWORK_PROV_SECURITY_1
    #define WIFI_PROV_EVENT_HANDLER_NONE        NETWORK_PROV_EVENT_HANDLER_NONE
    #define wifi_prov_mgr_config_t              network_prov_mgr_config_t
    #define wifi_prov_scheme_softap             network_prov_scheme_softap
    #define wifi_prov_mgr_init                  network_prov_mgr_init
    #define wifi_prov_mgr_start_provisioning    network_prov_mgr_start_provisioning
    #define wifi_prov_mgr_stop_provisioning     network_prov_mgr_stop_provisioning
    #define wifi_prov_mgr_is_provisioned        network_prov_mgr_is_wifi_provisioned
    #define wifi_prov_mgr_endpoint_create       network_prov_mgr_endpoint_create
    #define wifi_prov_mgr_wait                  network_prov_mgr_wait
    #define wifi_prov_mgr_endpoint_register     network_prov_mgr_endpoint_register
    #define wifi_prov_mgr_endpoint_unregister   network_prov_mgr_endpoint_unregister
    #define wifi_prov_mgr_deinit                network_prov_mgr_deinit

    #define tcpip_adapter_init()                esp_netif_t* sta_netif;\
                                                do{ esp_netif_init();\
                                                sta_netif = esp_netif_create_default_wifi_sta();\
                                                }while(0)
    #define tcpip_adapter_set_hostname(x,hostname)          esp_netif_set_hostname(sta_netif, hostname)
#endif

#define COMM_STACK_SIZE 4096

#define NETWORK_CONNECTED 0x01
#define NETWORK_LOST      0x02
#define WIFI_DISCONNECTED 0x04

#define MIDNIGHT          ((24<<8) | 0)  /*MSB=24; LSB=00*/

/*Define the cron entry structure (same as in nvCron.c)*/
typedef struct{
  U16 time;
  U8 func;
  U8 arg;
} s_cronEntry;

extern char device_uname[]; /*Defined in main.c*/

static const char TAG[] = "comm.c";
static TimerHandle_t wifiTimer;
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

static void wifiTimerCb(TimerHandle_t unused)
{
    xTimerDelete(wifiTimer, 0);
    ESP_ERROR_CHECK(esp_wifi_connect());
}

static void wifi_cb(void* self, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "wifi_cb:%d", event_id);
    if(event_id == WIFI_EVENT_STA_START){
        ESP_ERROR_CHECK(esp_wifi_connect());
    }else if(event_id == WIFI_EVENT_STA_DISCONNECTED){
        wifiTimer = xTimerCreate("wifi", BOARD_CFG_WIFI_RETRY_DELAY/portTICK_PERIOD_MS, pdFALSE, (void*) NULL, wifiTimerCb);
        xTimerStart(wifiTimer, 0);
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
    tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, device_uname);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /*Initialize WiFi provisioning manager with softap scheme*/
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_softap,
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
        .app_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
    };

    do{
        /*Initialize provisioning manager*/
        ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
    
        // Create and register custom endpoint for cron data
        ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_create("sProbe"));
        ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_create("bcfgWr"));

        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, WIFI_PROV_STA_DISCONNECTED, &prov_cb, NULL));
    
        
        
        // Check if device is already provisioned
        ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
        if (provisioned) {
            ESP_LOGI(TAG, "Device already provisioned. Skipping provisioning and starting normal WiFi mode.");
            break;
        }
        else {
            ESP_LOGI(TAG, "Starting provisioning...");

            /* Generate unique service name and key from mac*/
            char* service_key = &device_uname[7];   /* service key is the mac address portion of device_uname, so we need to skip first 7 chars corresponding to BUTLER_*/
            char pop[] = "12345678";  /* Default value if CONFIG_POP is not defined */
            
            ESP_LOGI(TAG, "{\"ver\":\"v1\",\"name\":\"%s\",\"password\":\"%s\",\"pop\":\"%s\",\"transport\":\"softap\",\"security\":\"1\"}", device_uname, service_key, pop);

            /* Start provisioning with security version V1 (WIFI_PROV_SECURITY_1) */
            ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1,
                                                             pop,
                                                             device_uname,
                                                             service_key));
            ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_register("sProbe", stateProbe_prov_handler, NULL)); // Replaced timeWr with sProbe
            ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_register("bcfgWr", boardCfg_wr_handler, NULL));
            ESP_LOGI(TAG, "Heap after wifi_prov_mgr_endpoint_register: %d bytes", esp_get_free_heap_size());

            /* Wait for provisioning to complete */
            wifi_prov_mgr_wait();
        }
        ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

        wifi_prov_mgr_endpoint_unregister("sProbe");
        wifi_prov_mgr_endpoint_unregister("bcfgWr");
    

        /*Deinitialize provisioning manager*/
        wifi_prov_mgr_deinit();
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_LOGI(TAG, "Heap after wifi_prov_mgr_deinit: %d bytes", esp_get_free_heap_size());
    }while(!provisioned);

    /* Connect to Wi-Fi */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_cb, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_cb, NULL));

    /* Start Wi-Fi */
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
