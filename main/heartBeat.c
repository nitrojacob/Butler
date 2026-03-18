#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include "board_cfg.h"
#include "heartBeat.h"

static void heartBeat_led_off(void* handle);
static void heartBeat_led_on(void* handle);

static void heartBeat_led_on(void* _handle_)
{
  s_heartBeatHandle* handle = (s_heartBeatHandle*) _handle_;
  handle->cursor--;
  //sysTimer_addAlert(&(handle->alert), SYSTIMER_TYPE_ONESHOT, 10, heartBeat_led_off, handle);
  os_timer_setfn(&(handle->alert), heartBeat_led_off, handle);
  os_timer_arm(&(handle->alert), 50, 0);
  gpio_set_level(handle->pin, 0);
}

static void heartBeat_led_off(void* _handle_)
{
  s_heartBeatHandle* handle = (s_heartBeatHandle*) _handle_;
  os_timer_setfn(&(handle->alert), heartBeat_led_on, handle);
  if(handle->cursor <= 0){
    os_timer_arm(&(handle->alert), handle->period, 0);
    //sysTimer_addAlert(&(handle->alert), SYSTIMER_TYPE_ONESHOT, handle->period, heartBeat_led_on, handle);
    handle->cursor = handle->ticks;
  }
  else{
    os_timer_arm(&(handle->alert), 400, 0);
    //sysTimer_addAlert(&(handle->alert), SYSTIMER_TYPE_ONESHOT, 400, heartBeat_led_on, handle);
  }
  //CLEAR_BIT(*(handle->port), handle->pin);
  gpio_set_level(handle->pin, 1);
}


void heartBeat_init(s_heartBeatHandle* handle, uint32_t period, uint8_t ticks, uint8_t pin)
{
  handle->period = period;
  handle->ticks  = ticks;
  handle->cursor = ticks;
  handle->pin = pin;
  os_timer_setfn(&(handle->alert), heartBeat_led_on, handle);
  os_timer_arm(&(handle->alert), handle->period, 0);
  gpio_set_direction(handle->pin, GPIO_MODE_OUTPUT);
}


void heartBeat_reconfigure(s_heartBeatHandle* handle, uint32_t period, uint8_t ticks)
{
  handle->period = period;
  handle->ticks  = ticks;
}
