#ifndef ACTUATOR_MOCK_H
#define ACTUATOR_MOCK_H

#include <stdbool.h>

// Mock interface for actuator functions
void actuator_mock_reset(void);
void actuator_mock_set_on_called(bool called);
void actuator_mock_set_off_called(bool called);
void actuator_mock_set_reflectState_called(bool called);
bool actuator_mock_get_on_called(void);
bool actuator_mock_get_off_called(void);
bool actuator_mock_get_reflectState_called(void);
int actuator_mock_get_on_count(void);
int actuator_mock_get_off_count(void);
int actuator_mock_get_reflectState_count(void);
int actuator_mock_get_state(void);

#endif // ACTUATOR_MOCK_H