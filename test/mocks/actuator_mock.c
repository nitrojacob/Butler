#include "actuator_mock.h"

static bool on_called = false;
static bool off_called = false;
static bool state = false;
static int on_count = 0;
static int off_count = 0;

void actuator_mock_reset(void)
{
    on_called = false;
    off_called = false;
    state = false;
    on_count = 0;
    off_count = 0;
}

void actuator_mock_set_on_called(bool called)
{
    if(called)
        state = true;
}

void actuator_mock_set_off_called(bool called)
{
    if (called)
        state=false;
}

int actuator_mock_get_state(void)
{
    if (state)
        return 1;
    else
        return 0;
}

void actuator_mock_set_reflectState_called(bool called)
{
    if (called) {
        if (state){
            on_count++;
            on_called = true;
        }
        else {
            off_count++;
            off_called = true;
        }
    }
}

bool actuator_mock_get_on_called(void)
{
    return on_called;
}

bool actuator_mock_get_off_called(void)
{
    return off_called;
}

int actuator_mock_get_on_count(void)
{
    return on_count;
}

int actuator_mock_get_off_count(void)
{
    return off_count;
}
