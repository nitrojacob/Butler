/**
 * Entry point and misc. other functions for Project Butler
 */
#include <stdio.h>
#include <time.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_event.h>
#include <esp_log.h>

#include <lwip/apps/sntp.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <mqtt_client.h>

#include <driver/gpio.h>
#include "stateProbe.h"
#include "actuator.h"
#include "fota.h"
#include "tmu.h"
#include "nvCron.h"
#include "comm.h"
#include "board_cfg.h"
#include "trap.h"

static const char TAG[] = "main.c";
static volatile int lateInitCompleted = 0;

/**
 * @brief Convert reset reason to string for logging
 */
static const char* reset_reason_to_string(esp_reset_reason_t reason)
{
    switch (reason) {
        case ESP_RST_UNKNOWN:    return "Unknown";
        case ESP_RST_POWERON:    return "Power-on";
        case ESP_RST_EXT:        return "External";
        case ESP_RST_SW:         return "Software";
        case ESP_RST_PANIC:      return "Panic";
        case ESP_RST_INT_WDT:    return "Interrupt Watchdog";
        case ESP_RST_TASK_WDT:   return "Task Watchdog";
        case ESP_RST_WDT:        return "Watchdog";
        case ESP_RST_DEEPSLEEP:  return "Deep Sleep";
        case ESP_RST_BROWNOUT:   return "Brownout";
        case ESP_RST_SDIO:       return "SDIO";
        case ESP_RST_FAST_SW:    return "Fast Software";
        default:                 return "Unknown";
    }
}

static void lateInit(void * pvParameters)
{
  /*time_t now;
  struct tm lnow;*/
  ESP_LOGI(TAG, "Resuming communication late init...");
  lateInitCompleted = 1;

  char ntpServer[BOARD_CFG_URL_SIZE] = {0};
  boardCfg_get_ntpServer(ntpServer, BOARD_CFG_URL_SIZE);

  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, ntpServer);
  sntp_init();

  /*do{
    vTaskDelay(6000/portTICK_PERIOD_MS);
    time(&now);
    localtime_r(&now, &lnow);
    printf("%d\n", lnow.tm_year);
  }while(lnow.tm_year == 70);

  gState &= 0x3;
  heartBeat_reconfigure(&heartBeat, BOARD_CFG_HB_OFF_TIME, gState+1);*/
  stateProbe_init();
  fota_init();
  nvCronRemote_init();
  tmuRemote_init();
  trap_init();

  /*Kill*/
  vTaskDelete(NULL);
}


static void gotIP_cb(void* self, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  if(lateInitCompleted == 0)
    xTaskCreate(lateInit, "late-init", LATE_INIT_TASK_STACK_SIZE, NULL, LATE_INIT_TASK_PRIO, NULL);
  else
    tmu_updateRTC();
}

static void lostIP_cb(void* self, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  tmu_updateRTC();
}

static void tick_loop( void * pvParameters )
{
  time_t now;
  struct tm lnow;
  actuator_init();
  time(&now);
  localtime_r(&now, &lnow);
  nvCron_init(lnow.tm_hour, lnow.tm_min);
  while(1)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    time(&now);
    localtime_r(&now, &lnow);
    nvCron_tick(lnow.tm_hour, lnow.tm_min);
    //printf("time:%d:%d\n", lnow.tm_hour, lnow.tm_min);
    //printf(asctime(&lnow));
  }
}

void app_main()
{
  tmu_init();

  ESP_LOGI(TAG, "app_main started...");
  esp_reset_reason_t reset_reason = esp_reset_reason();
  ESP_LOGI(TAG, "Reset reason: %s", reset_reason_to_string(reset_reason));
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &gotIP_cb, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &lostIP_cb, NULL));
  
  lateInitCompleted = 0;
  commInit();
  xTaskCreate(tick_loop, "tick", TICK_TASK_STACK_SIZE, NULL, TICK_TASK_PRIO, NULL);

  //printf("Restarting now.\n");
  //esp_restart();
}
