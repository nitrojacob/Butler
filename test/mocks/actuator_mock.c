#include "actuator_mock.h"

static bool on_called = false;
static bool off_called = false;
static bool reflectState_called = false;
static int on_count = 0;
static int off_count = 0;
static int reflectState_count = 0;

void actuator_mock_reset(void)
{
    on_called = false;
    off_called = false;
    reflectState_called = false;
    on_count = 0;
    off_count = 0;
    reflectState_count = 0;
}

void actuator_mock_set_on_called(bool called)
{
    on_called = called;
    if (called) on_count++;
}

void actuator_mock_set_off_called(bool called)
{
    off_called = called;
    if (called) off_count++;
}

void actuator_mock_set_reflectState_called(bool called)
{
    reflectState_called = called;
    if (called) reflectState_count++;
}

bool actuator_mock_get_on_called(void)
{
    return on_called;
}

bool actuator_mock_get_off_called(void)
{
    return off_called;
}

bool actuator_mock_get_reflectState_called(void)
{
    return reflectState_called;
}

int actuator_mock_get_on_count(void)
{
    return on_count;
}

int actuator_mock_get_off_count(void)
{
    return off_count;
}

int actuator_mock_get_reflectState_count(void)
{
    return reflectState_count;
}