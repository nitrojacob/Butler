#ifndef FREERTOS_MOCK_H
#define FREERTOS_MOCK_H

#include <stdbool.h>

// Mock interface for FreeRTOS functions
void freertos_mock_reset(void);
void freertos_mock_set_task_created(bool created);
bool freertos_mock_get_task_created(void);
int freertos_mock_get_task_create_count(void);

#endif // FREERTOS_MOCK_H