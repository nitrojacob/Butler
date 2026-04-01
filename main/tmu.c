/*
 * Time management unit (tmu) built on top of the time.h standard library
 */

#include <stdio.h>
#include <time.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <sys/time.h>
#include <esp_log.h>
#include "stateProbe.h"
#include "board_cfg.h"
#ifdef BOARD_CFG_USE_DS1307
  #include "ds1307.h"
#endif /*BOARD_CFG_USE_DS1307*/

#include "tmu.h"


#ifdef BOARD_CFG_USE_DS1307
static struct s_rtcUpdateData{
  time_t now;
  struct tm lnow;
  int day;
} rtcInfo = {0};
#endif /*BOARD_CFG_USE_DS1307*/

#define TOKEN_GET "time/get"
#define TOKEN_SET "time/set"

static const char TAG[] = "tmu.c";

stateProbe_probe get, set;

static void tmu_get(const char* buffer, int length)
{
  time_t now;
  struct tm lnow;
  char buf[32];
  int len;
  time(&now);
  localtime_r(&now, &lnow);
  len = snprintf(buf, 32, "%s", asctime(&lnow));
  stateProbe_write(&get, buf, len);
}

void tmu_set(const char* buffer, int length)
{
  struct tm new_time = {0};
  char buf[32];

  if (length >= sizeof(buf)) {
    ESP_LOGE(TAG, "Input time string is too long");
    return;
  }
  strncpy(buf, buffer, length);
  buf[length] = 0x0;

  if (strptime(buf, "%a %b %d %H:%M:%S %Y", &new_time) == NULL) {
    ESP_LOGE(TAG, "Failed to parse time string: %s", buf);
    return;
  }
  
  /* Convert struct tm to time_t */
  time_t new_timestamp = mktime(&new_time);
  if (new_timestamp == (time_t)-1) {
    ESP_LOGE(TAG, "Failed to convert time to timestamp");
    return;
  }
  
  /* Set system time */
  struct timeval tv;
  tv.tv_sec = new_timestamp;
  tv.tv_usec = 0;
  
  if (settimeofday(&tv, NULL) != 0) {
    ESP_LOGE(TAG, "Failed to set system time");
  } else {
    ESP_LOGI(TAG, "System time successfully set to: %s", buf);
  }
  tmu_updateRTC();
}

#ifdef BOARD_CFG_USE_DS1307
static void update_ds1307_task(void * noArg)
{
  time(&(rtcInfo.now));
  localtime_r(&(rtcInfo.now), &(rtcInfo.lnow));
  if(rtcInfo.lnow.tm_mday != rtcInfo.day){  /*Update time once in a day*/
    ds1307_set_time(&(rtcInfo.lnow));
    rtcInfo.day = rtcInfo.lnow.tm_mday;
    ESP_LOGI(TAG, "DS1307 updated at %s", asctime(&(rtcInfo.lnow)));
  }
  vTaskDelete(NULL);
}
#endif /*BOARD_CFG_USE_DS1307*/

void tmuRemote_init(void)
{
  
}

void tmu_updateRTC(void)
{
#ifdef BOARD_CFG_USE_DS1307
  xTaskCreate(update_ds1307_task, "ds1307", DS1307_TASK_STACK_SIZE, NULL, DS1307_TASK_PRIO, NULL);
#endif /*BOARD_CFG_USE_DS1307*/
}

void tmu_init(void)
{
  setenv("TZ", "UTC-05:30", 1);
  tzset();

#ifdef BOARD_CFG_USE_DS1307
  ds1307_init();
  if(ds1307_get_time(&(rtcInfo.lnow)) == ESP_OK)
  {
    ESP_LOGW(TAG, "DS1307 time:%s", asctime(&(rtcInfo.lnow)));
    rtcInfo.now = mktime(&(rtcInfo.lnow));
    struct timeval timeval;
    timeval.tv_sec = rtcInfo.now;
    timeval.tv_usec = 0;
    settimeofday(&timeval, NULL);
  }
  else
    ESP_LOGW(TAG, "DS1307 not found");
  rtcInfo.day = -1;
#endif /*BOARD_CFG_USE_DS1307*/
  stateProbe_register(&get, TOKEN_GET, tmu_get);
  stateProbe_register(&set, TOKEN_SET, tmu_set);
}