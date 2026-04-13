#include "freertos_mock.h"

static bool task_created = false;
static int task_create_count = 0;

// Minimal xTaskCreate stub used by tmu_updateRTC in tests. Runs the task function synchronously.
int xTaskCreate(void (*task)(void *), const char* name, unsigned short stackDepth, void* params, unsigned int priority, void* handle)
{
    task_created = true;
    task_create_count++;
    if (task) {
        task(params);
    }
    return 1; // pdTRUE
}

void freertos_mock_reset(void)
{
    task_created = false;
    task_create_count = 0;
}

void freertos_mock_set_task_created(bool created)
{
    task_created = created;
    if (created) {
        task_create_count++;
    }
}

bool freertos_mock_get_task_created(void)
{
    return task_created;
}

int freertos_mock_get_task_create_count(void)
{
    return task_create_count;
}