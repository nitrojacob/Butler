#include <driver/gpio.h>
#include "board_cfg.h"
#include "heartBeat.h"

#define MAX_WAIT (1000/portTICK_PERIOD_MS)
#define ON_DURATION (50/portTICK_PERIOD_MS)
#define OFF_SHORT (400/portTICK_PERIOD_MS)

static void heartBeat_led_off(s_heartBeatHandle* handle);
static void heartBeat_led_on(s_heartBeatHandle* handle);

static void heartBeat_led_on(s_heartBeatHandle* handle)
{
  handle->cursor--;
  //sysTimer_addAlert(&(handle->alert), SYSTIMER_TYPE_ONESHOT, 10, heartBeat_led_off, handle);
  handle->nextFunction = heartBeat_led_off;
  xTimerChangePeriod(handle->alert, ON_DURATION, MAX_WAIT);
  xTimerStart(handle->alert, MAX_WAIT);
  gpio_set_level(handle->pin, 0);
}

static void heartBeat_led_off(s_heartBeatHandle * handle)
{ 
  handle->nextFunction = heartBeat_led_on;
  if(handle->cursor <= 0){
    xTimerChangePeriod(handle->alert, handle->period/portTICK_PERIOD_MS, MAX_WAIT);
    xTimerStart(handle->alert, MAX_WAIT);
    //sysTimer_addAlert(&(handle->alert), SYSTIMER_TYPE_ONESHOT, handle->period, heartBeat_led_on, handle);
    handle->cursor = handle->ticks;
  }
  else{
    xTimerChangePeriod(handle->alert, OFF_SHORT, MAX_WAIT);
    xTimerStart(handle->alert, MAX_WAIT);
    //sysTimer_addAlert(&(handle->alert), SYSTIMER_TYPE_ONESHOT, 400, heartBeat_led_on, handle);
  }
  //CLEAR_BIT(*(handle->port), handle->pin);
  gpio_set_level(handle->pin, 1);
}

static void heartBeat_common_cb(TimerHandle_t _handle_)
{
  s_heartBeatHandle* handle = (s_heartBeatHandle*)pvTimerGetTimerID(_handle_);
  ((heartBeat_cb*)(handle->nextFunction))(handle);
}


void heartBeat_init(s_heartBeatHandle* handle, uint32_t period, uint8_t ticks, uint8_t pin)
{
  handle->period = period;
  handle->ticks  = ticks;
  handle->cursor = ticks;
  handle->pin = pin;
  handle->nextFunction = heartBeat_led_on;
  handle->alert = xTimerCreate("hb", handle->period/portTICK_PERIOD_MS, pdFALSE, (void*) handle, heartBeat_common_cb);
  xTimerStart(handle->alert, handle->period/portTICK_PERIOD_MS);
  gpio_set_direction(handle->pin, GPIO_MODE_OUTPUT);
}


void heartBeat_reconfigure(s_heartBeatHandle* handle, uint32_t period, uint8_t ticks)
{
  handle->period = period;
  handle->ticks  = ticks;
}
