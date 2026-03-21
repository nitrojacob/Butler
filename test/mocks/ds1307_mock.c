#include "ds1307_mock.h"
#include <time.h>

static bool ds1307_present = true;
static time_t ds1307_current_time = 0;
static int set_time_count = 0;
static int get_time_count = 0;

void ds1307_mock_reset(void)
{
    ds1307_present = true;
    ds1307_current_time = 0;
    set_time_count = 0;
    get_time_count = 0;
}

void ds1307_mock_set_present(bool present)
{
    ds1307_present = present;
}

bool ds1307_mock_is_present(void)
{
    return ds1307_present;
}

void ds1307_mock_set_time(time_t time)
{
    ds1307_current_time = time;
    set_time_count++;
}

time_t ds1307_mock_get_time(void)
{
    get_time_count++;
    return ds1307_current_time;
}

int ds1307_mock_get_set_time_count(void)
{
    return set_time_count;
}

int ds1307_mock_get_get_time_count(void)
{
    return get_time_count;
}