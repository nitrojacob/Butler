/* 
 * Implementation of trap module to handle clearButton callback
 * and stateProbe registration
 */

#include <stdio.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_err.h>

#include "trap.h"
#include "clearButton.h"

static const char TAG[] = "trap.c";

/**
 * @brief Original clear button reset implementation
 * This function performs the actual reset logic
 */
static void cfg_reset(void)
{
    ESP_LOGE(TAG, "---------Resetting provisioned state----------");
    ESP_ERROR_CHECK(nvs_flash_deinit());
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // Delay a bit to allow log to be printed
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    // Reset the device
    ESP_LOGE(TAG, "---------Resetting device----------");
    esp_restart();
}

/**
 * @brief Wrapper function for stateProbe callback
 * This matches the signature expected by stateProbe_register (probeFn)
 */
static void cfgReset_probe_cb(const char* data, int length)
{
    // The stateProbe system will call this function with MQTT data
    // We don't need to use the data, just trigger the reset
    cfg_reset();
}

/**
 * @brief Wrapper function for GPIO event handler
 * This matches the signature expected by esp_event_handler_register
 */
static void clearButton_cb(void* self, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    // This is called by the GPIO interrupt handler
    cfg_reset();
}

/**
 * @brief State probe structure for clear button functionality
 */
static stateProbe_probe clearButtonProbe;

/**
 * @brief Initialize trap module and register clearButton_cb with stateProbe
 */
void trap_init(void)
{
    /* Register clear button event handler with stateProbe */
    stateProbe_register(&clearButtonProbe, "cfgReset", &cfgReset_probe_cb);
    ESP_ERROR_CHECK(esp_event_handler_register(GPIO_INTERRUPT_EVENTS, GPIO_0_INTERRUPT_EVENT, &clearButton_cb, NULL));
}