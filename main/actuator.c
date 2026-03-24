/* For ESP8266 NodeMCU board
 * Actuator number is what user and external apps refer to.
 * Conversion to GPIO is done using actuatorGPIOMap array
 */

#include <stdio.h>
#include <driver/gpio.h>
#include "stateProbe.h"
#include "trap.h"
#include "board_cfg.h"
#include "actuator.h"

#ifndef ACTUATOR_PRIO_NOF
  #error "ACTUATOR_PRIO_NOF not defined. Define it at board_cfg.h or at commandline"
#endif /*ACTUATOR_PRIO_NOF*/

/* The number of actuator ports. Minimum value = 2.
 * Last actuator port is for soft actuators like PWM etc
 */
#define ACTUATOR_PORTS_NOF  2
#define ACTUATOR_PINS_PER_PORT 8

volatile static U8 actuatorState[ACTUATOR_PRIO_NOF][ACTUATOR_PORTS_NOF];
static const U8 actuatorGPIOMap[ACTUATOR_PINS_PER_PORT] = {
    /*actuator*/    /*GPIO*/
    /*0*/             5,  /*HighZ throughout boot*/
    /*1*/             4,  /*"*/
    /*2*/             13, /*High during boot*/
    /*3*/             15,
    /*4*/             15,
    /*5*/             15,
    /*6*/             15,
    /*7*/             15
};
static softActFn softActuator[ACTUATOR_PINS_PER_PORT];
U8 softActuatorShadow, pinActuatorShadow;

void actuator_reflectState(void)
{
  int i;
  U8 change, softActuatorPortIdx;
  U8 finalState[ACTUATOR_PORTS_NOF];
  for(i=0; i<ACTUATOR_PORTS_NOF; i++)
    finalState[i] = 0xff;
  for(i=0; i<ACTUATOR_PRIO_NOF; i++)
  {
    finalState[0] &= actuatorState[i][0];
    finalState[1] &= actuatorState[i][1];
  }

  change = pinActuatorShadow ^ finalState[0];
  if(change)
  {
    for(i=0; i<ACTUATOR_PINS_PER_PORT; i++)
    {
      if((change & 0x01)){
        gpio_set_level(actuatorGPIOMap[i], (finalState[0] >> i) & 0x01);
        BUTLER_LOG("Pin %d set to %d", i, (finalState[0] >> i) & 0x01);
      }
      change >>= 1;
    }
  }
  pinActuatorShadow = finalState[0];
  softActuatorPortIdx = ACTUATOR_PORTS_NOF - 1;
  change = softActuatorShadow ^ finalState[softActuatorPortIdx];
  if(change)
  {
    for(i=0; i<ACTUATOR_PINS_PER_PORT; i++)
    {
      if((change & 0x01) && (softActuator[i]!=NULL))
        softActuator[i](((~finalState[softActuatorPortIdx]) >> i) & 0x01);
      change >>= 1;
    }
  }
  softActuatorShadow = finalState[softActuatorPortIdx];
}

void actuator_on(U8 actuator, U8 priority)
{
  CLEAR_BIT(actuatorState[priority][(actuator & 0xf0) >> 4], (actuator&0x07));
}
void actuator_off(U8 actuator, U8 priority)
{
  SET_BIT(actuatorState[priority][(actuator & 0xf0) >> 4], (actuator&0x07));
}
void actuator_flip(U8 actuator, U8 priority)
{
  TOGGLE_BIT(actuatorState[priority][(actuator & 0xf0) >> 4], (actuator&0x07));
}

void actuator_init(void)
{
  int i, j;
  for(i=0; i<ACTUATOR_PRIO_NOF; i++)
    for(j=0; j<ACTUATOR_PORTS_NOF; j++)
      actuatorState[i][j] = ~0;
  softActuatorShadow = ~0;
  pinActuatorShadow = ~0;
  for(i=0; i<ACTUATOR_PINS_PER_PORT; i++)
    softActuator[i] = NULL;
  for(i=0; i<ACTUATOR_PINS_PER_PORT; i++)
  {
    gpio_set_level(actuatorGPIOMap[i], (pinActuatorShadow>>i) & 0x01);
    gpio_config_t led;
    led.intr_type = GPIO_INTR_DISABLE;
    led.mode = GPIO_MODE_OUTPUT;
    led.pin_bit_mask = 1 << actuatorGPIOMap[i];
    led.pull_down_en = GPIO_PULLDOWN_DISABLE;
    led.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&led);
    //gpio_set_level(actuatorGPIOMap[i], (pinActuatorShadow>>i) & 0x01);
  }
  actuator_reflectState();
}

void actuator_registerSoft(U8 index, softActFn pCallback)
{
  softActuator[index] = pCallback;
}

#if defined(DEBUG_STATE_PROBE) && defined(DEBUG_ACTUATOR)
/*Need to register this callback*/
void actuator_dumpState(void)
{
  int i, j;
  for(i=0; i<ACTUATOR_PRIO_NOF; i++)
  {
    stateProbe_printf("P[prio=%d]:", i);
    for(j=0; j<ACTUATOR_PORTS_NOF; j++)
      stateProbe_printf("%x,", actuatorState[i][j]);
    stateProbe_printf("\n");
  }
}
#else
void actuator_dumpState(void)
{

}
#endif
