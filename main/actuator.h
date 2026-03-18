#ifndef __ACTUATOR_H__
#define __ACTUATOR_H__

#include "fixedTypes.h"

typedef void (*softActFn)(U8);

void actuator_init(void);

void actuator_on(U8 actuator, U8 priority);
void actuator_off(U8 actuator, U8 priority);
void actuator_flip(U8 actuator, U8 priority);

/**@brief Makes the internal actuatorState reflect on the external port electrically*/
void actuator_reflectState(void);

/**@brief Registers a soft actuator callback function. Soft actuators are functions that will be called back when
 * someone asks to turn on or off. The function may turn on/off a certain monitoring algorithm along with associated driver.
 * Basically things more complex than driving a relay/led/alarm can be done through this interface
 */
void actuator_registerSoft(U8 index, softActFn pCallback);

void actuator_dumpState(void);

#endif /*__ACTUATOR_H__*/
