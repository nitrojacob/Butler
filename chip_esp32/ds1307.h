/**
 * esp32/ds1307.h
 * ESP32-specific DS1307 RTC interface.
 * Provides the same public API as main/ds1307.h but implemented for esp-idf.
 */
#ifndef __ESP32_DS1307_H__
#define __ESP32_DS1307_H__

#include <time.h>
#include "esp_err.h"

void ds1307_init(void);
esp_err_t ds1307_get_time(struct tm *tm);
esp_err_t ds1307_set_time(struct tm *tm);

#endif /* __ESP32_DS1307_H__ */
