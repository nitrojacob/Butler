#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include "clearButton.h"

#include "board_cfg.h"

#define CLEARBUTTON_DEBOUNCE_MS 3000

/* Custom event base for clear button events */
ESP_EVENT_DEFINE_BASE(GPIO_INTERRUPT_EVENTS);

static volatile TickType_t last_press_tick = 0;
static volatile uint8_t last_press_seen = 0;


/* Top-half ISR handler */
void IRAM_ATTR clearButton_isr_handler(void* arg)
{
    /* Post event to the default event loop from ISR context */
    /* This is the top-half - minimal work here */
    /* Determine whether this is a press (active low => 0) or release (1).
     * Record press time and only post the event on a release if the
     * elapsed time since press exceeds the debounce threshold. */
    BaseType_t higher_priority_task_woken = pdFALSE;

    int level = gpio_get_level(BOARD_CFG_PIN_NVS_RESET);

    const TickType_t debounce_ticks = pdMS_TO_TICKS(CLEARBUTTON_DEBOUNCE_MS);

    if (level == 0) {
        /* Button pressed (active low) - record time */
        last_press_tick = xTaskGetTickCountFromISR();
        last_press_seen = 1;
    } else {
        /* Button released - check debounce */
        if (last_press_seen) {
            TickType_t now = xTaskGetTickCountFromISR();
            TickType_t delta = now - last_press_tick;
            last_press_seen = 0;
            if (delta >= debounce_ticks) {
                esp_event_isr_post(GPIO_INTERRUPT_EVENTS, GPIO_0_INTERRUPT_EVENT, NULL, 0, &higher_priority_task_woken);
            }
        }
    }

    /* If a higher priority task was unblocked, yield to it */
    if (higher_priority_task_woken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

/* Initialize the clear button on GPIO0 */
void clearButton_init(void)
{
    /* Configure GPIO0 as input with pull-up */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOARD_CFG_PIN_NVS_RESET),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        /* Watch both edges: press (falling) and release (rising) */
        .intr_type = GPIO_INTR_ANYEDGE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    /* Install GPIO ISR service */
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    
    /* Register the ISR handler */
    ESP_ERROR_CHECK(gpio_isr_handler_add(BOARD_CFG_PIN_NVS_RESET, clearButton_isr_handler, NULL));
}
