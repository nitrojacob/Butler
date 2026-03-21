#ifndef __CLEARBUTTON_H__
#define __CLEARBUTTON_H__

#include <stdint.h>
#include <esp_event.h>

/* Event base declaration */
extern esp_event_base_t GPIO_INTERRUPT_EVENTS;

typedef enum {
    GPIO_0_INTERRUPT_EVENT = 0,
}e_GPIO_INTERRUPT_EVENT;

/* Public API functions */
void clearButton_init(void);

#endif /* __CLEARBUTTON_H__ */
