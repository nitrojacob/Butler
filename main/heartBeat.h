#ifndef __HEARTBEAT_H__
#define __HEARTBEAT_H__

typedef struct{
  os_timer_t      alert;
  uint32_t        period;
  uint8_t         ticks;
  uint8_t         cursor;
  uint8_t         pin;
}s_heartBeatHandle;

/**
 * @brief Initialises a heartBeat indicator (a blinking led) on the specified port and offset.
 *
 * @arg
 * handle: The user has to allocate memory for a variable of type s_heartBeatHandle and provide the pointer to this function.
 * period: The off-time of the LED (in milliseconds)
 * ticks : No of 10ms blinks between off-times
 * port: PORTA|PORTB|PORTC|PORTD
 * ddr : DDRA|DDRB|DDRC|DDRD
 * offset: 0-7 pin's offset within the port
 */
void heartBeat_init(s_heartBeatHandle* handle, uint32_t period, uint8_t ticks, uint8_t pin);

/**
 * @brief Reconfigures the timing behaviour of a hearbeat indicator
 */
void heartBeat_reconfigure(s_heartBeatHandle* handle, uint32_t period, uint8_t ticks);

#endif /*__HEARTBEAT_H__*/
