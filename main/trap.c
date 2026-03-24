/* 
 * Implementation of trap module to handle clearButton callback
 * and stateProbe registration
 */

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_err.h>

#include "trap.h"
#include "clearButton.h"
#include "nvLogRing.h"
#include "stateProbe.h"

static const char TAG[] = "trap.c";
/**
 * @brief State probe structure for clear button functionality
 */
static stateProbe_probe clearButtonProbe;

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
 * @brief Initialize trap module and register clearButton_cb with stateProbe
 */
void trap_init(void)
{
    clearButton_init();
    ESP_ERROR_CHECK(esp_event_handler_register(GPIO_INTERRUPT_EVENTS, GPIO_0_INTERRUPT_EVENT, &clearButton_cb, NULL));
}

void trap_late_init(void)
{
    /* Register clear button event handler with stateProbe */
    stateProbe_register(&clearButtonProbe, "cfgReset", &cfgReset_probe_cb);
}

/**
 * @brief Internal logging helper that prefixes a timestamp and forwards
 * the message to nvLogRing and stateProbe.
 *
 * The final message will be truncated to NVLOGRING_MAX_ENTRY_LENGTH bytes
 * (not including the implicit NUL) to fit nvLogRing.
 */
void trap_butler_log(const char* fmt, ...)
{
    char msgbuf[NVLOGRING_MAX_ENTRY_LENGTH + 1];
    char tsbuf[20]; /* yyyy-mm-dd HH:MM => 16 + spare */
    time_t now = time(NULL);
    struct tm tm;

    /* Get local time; if not available, zero the tm */
    if (localtime_r(&now, &tm) == NULL) {
        tm.tm_year = 70; tm.tm_mon = 0; tm.tm_mday = 1; tm.tm_hour = 0; tm.tm_min = 0;
    }

    /* Format timestamp: yyyy-mm-dd HH:MM */
    if (strftime(tsbuf, sizeof(tsbuf), "%Y-%m-%d %H:%M", &tm) == 0) {
        tsbuf[0] = '\0';
    }

    /* Format the user message into a temporary buffer */
    char userbuf[NVLOGRING_MAX_ENTRY_LENGTH + 1];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(userbuf, sizeof(userbuf), fmt, ap);
    va_end(ap);

    /* Compose final message: "yyyy-mm-dd HH:MM <usermsg>".
     * Reserve 1 char for space and 1 for NUL. Limit the number of
     * characters copied from userbuf so snprintf cannot overflow. */
    if (tsbuf[0] != '\0') {
        size_t tslen = strlen(tsbuf);
        /* max bytes left for user message (not counting NUL and space) */
        int max_user = (int)(sizeof(msgbuf) - tslen - 2);
        if (max_user < 0) max_user = 0;
        /* Use precision to limit copied characters from userbuf */
        snprintf(msgbuf, sizeof(msgbuf), "%s %.*s", tsbuf, max_user, userbuf);
    } else {
        /* No timestamp available */
        snprintf(msgbuf, sizeof(msgbuf), "%s", userbuf);
    }

    /* Ensure null termination */
    msgbuf[sizeof(msgbuf) - 1] = '\0';

    /* Write to non-volatile ring and state probe. stateProbe_log expects char* */
    nvLogRing_write(msgbuf);
    stateProbe_log(msgbuf);
}