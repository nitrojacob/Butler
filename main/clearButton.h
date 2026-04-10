#ifndef __CLEARBUTTON_H__
#define __CLEARBUTTON_H__

#include <stdint.h>
#include <esp_event.h>

#ifdef CONFIG_IDF_TARGET_ESP8266
    /* Event base declaration */
    extern esp_event_base_t GPIO_INTERRUPT_EVENTS;
#else
    /* Event base declaration */
    extern esp_event_base_t const GPIO_INTERRUPT_EVENTS;
#endif

typedef enum {
    GPIO_0_INTERRUPT_EVENT = 0,
}e_GPIO_INTERRUPT_EVENT;

/* Public API functions */
void clearButton_init(void);

#endif /* __CLEARBUTTON_H__ */
