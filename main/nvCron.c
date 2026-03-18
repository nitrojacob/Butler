#include <string.h>
#include <nvs.h>
#include <esp_log.h>

#include "stateProbe.h"
#include "actuator.h"
#include "board_cfg.h"
#include "nvCron.h"

#ifndef ACTUATOR_NVCRON_PRIO
  #error "ACTUATOR_NVCRON_PRIO not defined. Expected it to be defined in board_cfg.h"
#endif /*NVCRON_ACTUATOR_PRIO*/

#ifndef CRON_MAX_ENTRIES
  #error "CRON_MAX_ENTRIES not defined. Expected it to be defined in board_cfg.h"
#endif /*CRON_MAX_ENTRIES*/

#define MIDNIGHT          ((24<<8) | 0)  /*MSB=24; LSB=00*/



typedef struct{
  U16 time;
  U8 func;
  U8 arg;
}s_cronEntry;

#ifdef ENABLE_NVCRON_REMOTE
static stateProbe_probe cronProbeRd, cronProbeWr;
static const char nvCronRd[] = "nvCron/rd";
static const char nvCronWr[] = "nvCron/wr";
#endif /*ENABLE_NVCRON_REMOTE*/

static U16 time_next = MIDNIGHT;
static U16 time_now = MIDNIGHT;

static nvs_handle_t nvs = (nvs_handle_t)0;

static const char TAG[] = "nvCron.c";

static void nvCron_updateState(s_cronEntry* entry)
{
  U8 arg = entry->arg;
  switch((e_cronFunc)entry->func)
  {
    case CRON_FUNC_RELAY_OFF: actuator_off(arg, ACTUATOR_NVCRON_PRIO);
      break;
    case CRON_FUNC_RELAY_ON: actuator_on(arg, ACTUATOR_NVCRON_PRIO);
      break;
    default:
      break;
  }
}

/**@brief find the immediate next time where a task is due. Also calls out any task due now*/
static U16 nvCron_traverseList(U16 t_now)
{
  U16 i;
  U16 t_entry;
  U16 d_lowest = MIDNIGHT;
  U16 idx_lowest = CRON_MAX_ENTRIES;
  char key[7];
  s_cronEntry candidate;
  for(i=0; i<CRON_MAX_ENTRIES; i++)
  {
    snprintf(key, 7, "cron%02d", i);
    size_t size = sizeof(s_cronEntry);
    esp_err_t nvsErr = nvs_get_blob(nvs, key , &candidate, &size);
    if((candidate.time >= MIDNIGHT) || (candidate.func >= (U8)CRON_FUNC_NOF) || (nvsErr == ESP_ERR_NVS_NOT_FOUND)) /*Skip invalid/blank entries*/
      continue;
    t_entry = (candidate.time > t_now)?candidate.time:(candidate.time + MIDNIGHT);
    if((t_entry - t_now) < d_lowest)
    {
      d_lowest = (t_entry - t_now);
      idx_lowest = i;
    }
    if(t_now == candidate.time)
      nvCron_updateState(&candidate);
  }
  if(idx_lowest < CRON_MAX_ENTRIES){
    snprintf(key, 7, "cron%02d", idx_lowest);
    size_t size = sizeof(s_cronEntry);
    nvs_get_blob(nvs, key, &candidate, &size);
  }
  else
    candidate.time = MIDNIGHT;
  return candidate.time;
}

static U16 nvCron_initState(U16 t_now)
{
  U16 true_next;
  true_next = nvCron_traverseList(t_now);
  t_now = true_next;
  do{     /*Cycle a full day through the cornlist to accumulate the activations into state*/
    t_now = nvCron_traverseList(t_now);
  }while(t_now < MIDNIGHT && true_next < MIDNIGHT && true_next != t_now);
  return true_next;
}

void nvCron_writeMultipleEntry(const char* inbuf, int length)
{
  /* Complex handling is required to prevent ambiguous configurations like same function having multiple actions at identical time */
  U16 i, j;
  char key[7];
  size_t size = sizeof(s_cronEntry);
  s_cronEntry candidate, buf[CRON_MAX_ENTRIES];
  int emptyIdx;
  U8 isDuplicate;
  U8 isCurrent[CRON_MAX_ENTRIES] = {0};
  int count = length/size;
  if (count > CRON_MAX_ENTRIES)
    count = CRON_MAX_ENTRIES; /*Truncate to max possible entries*/
  memcpy(buf, inbuf, count * size);

  for (j=0; j < count; j++)
  {
    /*Addition Pass*/
    /* If Entry to add is invalid/empty, then skip */
    if((buf[j].time >= MIDNIGHT || buf[j].func >= (U8)CRON_FUNC_NOF))
      continue; /*Invalid Entry*/

    /* j th entry is valid. Let's find an empty slot to add it. */
    emptyIdx = CRON_MAX_ENTRIES;
    isDuplicate = 0;
    for(i=0; i < CRON_MAX_ENTRIES; i++)
    {
      snprintf(key, 7, "cron%02d", i);
      esp_err_t nvsErr = nvs_get_blob(nvs, key, &candidate, &size);
      if((candidate.time >= MIDNIGHT) || (candidate.func >= (U8)CRON_FUNC_NOF) || (nvsErr == ESP_ERR_NVS_NOT_FOUND)) /*Skip invalid/blank entries*/
      {
        if (emptyIdx >= CRON_MAX_ENTRIES)
          emptyIdx = i;
        continue;
        /* Found the first empty slot, and have stored it to emptyIdx */
      }
      if(candidate.time == buf[j].time && candidate.func == buf[j].func)
      {
        /*Duplicate found -- overwrite the entry*/
        ESP_ERROR_CHECK(nvs_set_blob(nvs, key, &buf[j], size));
        /*ESP_LOGI(TAG, "Duplicate found. Overwriting %s: time=%x, func=%d, arg=%d", key, buf[j].time, buf[j].func, buf[j].arg);*/
        isDuplicate = 1;
        isCurrent[i]++;
        break;
      }
    }
    if(!isDuplicate)
    {
      snprintf(key, 7, "cron%02d", emptyIdx);
      if(emptyIdx < CRON_MAX_ENTRIES){
        ESP_ERROR_CHECK(nvs_set_blob(nvs, key, &buf[j], size));
        ESP_LOGI(TAG, "Adding new entry %s: time=%x, func=%d, arg=%d", key, buf[j].time, buf[j].func, buf[j].arg);
        isCurrent[emptyIdx]++;
      }
    }
  }
  for(i=0; i < CRON_MAX_ENTRIES; i++)
  {
    /*Deletion Pass*/
    if(isCurrent[i])
      continue;
    snprintf(key, 7, "cron%02d", i);
    esp_err_t nvsErr = nvs_erase_key(nvs, key);
    if(nvsErr != ESP_OK && nvsErr != ESP_ERR_NVS_NOT_FOUND)
      ESP_LOGE(TAG, "Failed to erase %s, err=%s", key, esp_err_to_name(nvsErr));
    else if(nvsErr == ESP_OK)
      ESP_LOGI(TAG, "Deleted entry %s", key);
  }
  ESP_ERROR_CHECK(nvs_commit(nvs));
  time_next = nvCron_initState(time_now);    /*Initialise time_next & gpio state*/
  actuator_reflectState();
}


int nvCron_readMultipleEntry(char* buffer, int length)
{
  int i;
  char key[7];
  size_t size = sizeof(s_cronEntry);
  s_cronEntry buf[CRON_MAX_ENTRIES] = {0};
  int count = length/size;
  if (count > CRON_MAX_ENTRIES)
    count = CRON_MAX_ENTRIES; /*Truncate to max possible entries*/
  
  for(i=0; i<count; i++)
  {
    snprintf(key, 7, "cron%02d", i & 0x3f);
    esp_err_t nvsErr = nvs_get_blob(nvs, key, &buf[i], &size);
    if (nvsErr != ESP_OK && nvsErr != ESP_ERR_NVS_NOT_FOUND) {
      ESP_LOGW(TAG, "Failed to read %s, err=%s", key, esp_err_to_name(nvsErr));
    }
    if (nvsErr == ESP_ERR_NVS_NOT_FOUND || buf[i].time >= MIDNIGHT || buf[i].func >= (U8)CRON_FUNC_NOF) {
      buf[i].time = MIDNIGHT; // Mark as invalid entry
      buf[i].func = 0;
      buf[i].arg = 0;
    }
  }
  memcpy(buffer, buf, count * size);
  return count * size;
}

#ifdef ENABLE_NVCRON_REMOTE
static void cronEntryRead_cb(const char* buffer, int length)
{
  char rdBuf[CRON_MAX_ENTRIES * sizeof(s_cronEntry)] = {0};
  int rdBufLen = CRON_MAX_ENTRIES * sizeof(s_cronEntry);
  length = nvCron_readMultipleEntry(rdBuf, rdBufLen);
  stateProbe_write(&cronProbeRd, rdBuf, length);
}

void nvCronRemote_init(void)
{
  stateProbe_register(&cronProbeRd, nvCronRd, cronEntryRead_cb);
  stateProbe_register(&cronProbeWr, nvCronWr, nvCron_writeMultipleEntry);
}
#else /*ENABLE_NVCRON_REMOTE*/
void nvCronRemote_init(void)
{
}
#endif /*ENABLE_NVCRON_REMOTE*/

void nvCron_init(U8 hour, U8 minute)
{
  time_now = (((U16)hour) << 8) | minute;
  ESP_ERROR_CHECK(nvs_open("cron", NVS_READWRITE, &nvs));
  time_next = nvCron_initState(time_now);    /*Initialise time_next & gpio state*/
  actuator_reflectState();
}

void nvCron_tick(U8 hour, U8 minute)
{
  /*nvCron_initState() if time corrections detected*/
  U16 time_new = (((U16)hour) << 8) | minute;
  if(time_new == time_now)
  {
    /*time_new == time_now. The most common case. Time has not changed. Yet we got a call. No action required*/
  }
  else if((time_new == (time_now +1)) || (time_new == (time_now+197)))
  {
    /*The time incremented by 1 minute. We need to see if any action is due.*/
    time_now = time_new;

    if(time_now == time_next){
      time_next = nvCron_traverseList(time_now);
      actuator_reflectState();
    }
  }
  else
  {
    /* Jumps in time by more than 1 minute. eg. due to ntp update */
    time_next = nvCron_initState(time_new);
    actuator_reflectState();
    ESP_LOGW(TAG, "time update: New: %2d.%02d, Old: %2d.%02d, Next: %2d.%02d", time_new>>8, time_new&0xff, time_now>>8, time_now&0xff, time_next>>8, time_next&0xff);
    time_now = time_new;
  }
}

void nvCron_addEntry(U8 hour, U8 minute, e_cronFunc func, U8 arg)
{
  U16 i;
  s_cronEntry candidate;
  U16 emptyIndex = CRON_MAX_ENTRIES;
  U16 time = (((U16)hour) << 8) | minute;
  U8 isDuplicate = 0;
  char key[7];
  for(i=0; i<CRON_MAX_ENTRIES; i++)
  {
    snprintf(key, 7, "cron%02d", i);
    size_t size = sizeof(s_cronEntry);
    esp_err_t nvsErr = nvs_get_blob(nvs, key , &candidate, &size);
    if(candidate.time == time && candidate.func == (U8)func && candidate.arg == arg)
      isDuplicate = 1;
    if((candidate.time >= MIDNIGHT) || (candidate.func >= (U8)CRON_FUNC_NOF) || (nvsErr == ESP_ERR_NVS_NOT_FOUND)) /*Skip invalid/blank entries*/
      if(emptyIndex >= CRON_MAX_ENTRIES)
        emptyIndex = i;
  }
  candidate.time = time;
  candidate.func = (U8)func;
  candidate.arg  = arg;
  if(emptyIndex<CRON_MAX_ENTRIES && !isDuplicate){
    snprintf(key, 7, "cron%02d", emptyIndex);
    size_t size = sizeof(s_cronEntry);
    nvs_set_blob(nvs, key, &candidate, size);
  }
  nvs_commit(nvs);
}

void nvCron_delEntry(U8 hour, U8 minute, e_cronFunc func, U8 arg)
{
  U16 i;
  s_cronEntry candidate;
  U16 time = (((U16)hour) << 8) | minute;
  char key[7];
  for(i=0; i<CRON_MAX_ENTRIES; i++)
  {
    snprintf(key, 7, "cron%02d", i);
    size_t size = sizeof(s_cronEntry);
    nvs_get_blob(nvs, key, &candidate, &size);
    if((candidate.time == time) && (candidate.func == (U8)func) &&(candidate.arg == arg))
    {
      snprintf(key, 7, "cron%02d", i);
      nvs_erase_key(nvs, key);
    }
  }
  nvs_commit(nvs);
}

