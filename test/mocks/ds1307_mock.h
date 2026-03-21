#ifndef DS1307_MOCK_H
#define DS1307_MOCK_H

#include <stdbool.h>
#include <time.h>

// Mock interface for DS1307 RTC functions
void ds1307_mock_reset(void);
void ds1307_mock_set_present(bool present);
bool ds1307_mock_is_present(void);
void ds1307_mock_set_time(time_t time);
time_t ds1307_mock_get_time(void);
int ds1307_mock_get_set_time_count(void);
int ds1307_mock_get_get_time_count(void);

#endif // DS1307_MOCK_H