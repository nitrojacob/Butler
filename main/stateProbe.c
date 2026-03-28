/**@file Routines to enable a module to respond to state-probe events from the user
 * State probing is a debug mechanism, through which the engineer can request the target
 * over an interface(like UART) to dump a set of textual information about the state

 * TODO: WHEN THE CALLBACK OCCURS ADD THE PREFIX TO THE COMPARISON
 */

#include "fixedTypes.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <mqtt_client.h>
#include "board_cfg.h"
#include "stateProbe.h"
#include <stdio.h>

#ifndef PROBE_STACK_SIZE_MAX
  #define PROBE_STACK_SIZE_MAX 0xA0
#endif /*PROBE_STACK_SIZE_MAX*/

#ifndef PROBE_TASK_PRIORITY
  #define PROBE_TASK_PRIORITY 1
#endif /*PROBE_TASK_PRIORITY*/

#define MQTT_CONNECTED 0x01
#define MQTT_LOST      0x02

#define MQTT_TOPIC_LEN_MAX 54

static const char TAG[] = "stateProbe.c";
static EventGroupHandle_t mqtt_event;
static stateProbe_probe *probeList = NULL;
static esp_mqtt_event_handle_t context = NULL;
static esp_mqtt_client_handle_t client;
static char mqtt_name[]="butler0000000000000000/";
static char initDone = 0;

static void stateProbe_subscribePending(esp_mqtt_client_handle_t client)
{
  stateProbe_probe *pCurrent;
  char topic[MQTT_TOPIC_LEN_MAX] = {0};

  pCurrent = probeList;
  while(pCurrent != NULL)
  {
//Below line commented to fix "not subscribing to messages on re-connection to a broker after network loss"
//    if(pCurrent->subscribed == 0){
      snprintf(topic, MQTT_TOPIC_LEN_MAX, "%s%s", mqtt_name, pCurrent->probeSymbol);
      esp_mqtt_client_subscribe(client, topic, 0);
      pCurrent->subscribed = 1;
//    }
    pCurrent = pCurrent->pNext;
    //yield();
  }
}

static void mqtt_cb(void* self, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            stateProbe_subscribePending(client);
            xEventGroupSetBits(mqtt_event, MQTT_CONNECTED);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            xEventGroupSetBits(mqtt_event, MQTT_LOST);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            ESP_LOGD(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
            ESP_LOGD(TAG, "DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

static void stateProbe_probeCb(void* self, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  int len_mqtt_name;
  context = (esp_mqtt_event_handle_t)event_data;

  ESP_LOGI(TAG, "MQTT_EVENT_DATA - probeCb");
  stateProbe_probe *pCurrent;
  pCurrent = probeList;
  len_mqtt_name = strlen(mqtt_name);
  if(strncmp(context->topic, mqtt_name, len_mqtt_name) == 0)    /*Compare the device name*/
  {
    while(pCurrent != NULL)
    {
      if(strncmp(&(context->topic[len_mqtt_name]), pCurrent->probeSymbol, strlen(pCurrent->probeSymbol)) == 0)    /*compare the subtopic*/
        pCurrent->pProbeFn(context->data, context->data_len);
      pCurrent = pCurrent->pNext;
    }
  }
  context = NULL;
}

void stateProbe_init(void)
{
  uint8_t mac[8] = {0};
  mqtt_event = xEventGroupCreate();
  char mqttBroker[BOARD_CFG_URL_SIZE] = {0};
  boardCfg_get_mqttBroker(mqttBroker, BOARD_CFG_URL_SIZE);
  esp_mqtt_client_config_t mqtt_cfg = {
        .host = mqttBroker,
        .transport = MQTT_TRANSPORT_OVER_TCP,
        .client_id = mqtt_name,
  };
  esp_efuse_mac_get_default(mac);
  snprintf(mqtt_name, sizeof(mqtt_name), "butler%02x%02x%02x%02x%02x%02x%02x%02x/", mac[0], mac[1],mac[2], mac[3], mac[4], mac[5], mac[6], mac[7]);
  ESP_LOGI(TAG, "MQTT NAME=%s", mqtt_name);
  client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_cb, client);
  esp_mqtt_client_register_event(client, MQTT_EVENT_DATA, stateProbe_probeCb, client);
  esp_mqtt_client_start(client);

  xEventGroupWaitBits(mqtt_event, MQTT_CONNECTED, pdFALSE, pdFALSE, portMAX_DELAY);
  stateProbe_subscribePending(client);
  initDone = 1;
}

void stateProbe_startTask(void)
{
}

void stateProbe_register(stateProbe_probe* pProbe, const char* probeSymbol, probeFn pProbeFunction)
{
  pProbe->pNext = probeList;
  pProbe->probeSymbol = probeSymbol;
  pProbe->pProbeFn = pProbeFunction;
  pProbe->subscribed = 0;
  probeList = pProbe;
  stateProbe_subscribePending(client);
}

void stateProbe_write(stateProbe_probe* probe, char* buf, int len)
{
  char topic[MQTT_TOPIC_LEN_MAX+4];
  esp_mqtt_client_handle_t client = context->client;
  snprintf(topic, MQTT_TOPIC_LEN_MAX+4, "%s%sData", mqtt_name, probe->probeSymbol);
  esp_mqtt_client_publish(client, topic, buf, len, 0, 0);
}

void stateProbe_log(char* log)
{
  char topic[MQTT_TOPIC_LEN_MAX];
  if(initDone){
    /* Initialization done client variable has valid content*/
    snprintf(topic, MQTT_TOPIC_LEN_MAX+4, "%slog", mqtt_name);
    esp_mqtt_client_publish(client, topic, log, strlen(log), 0, 0);
  }
}
