#ifndef __DS1307_H__
#define __DS1307_H__

#include <time.h>

void ds1307_init(void);
esp_err_t ds1307_get_time(struct tm *tm);
esp_err_t ds1307_set_time(struct tm *tm);
#endif /* __DS1307_H__ */
