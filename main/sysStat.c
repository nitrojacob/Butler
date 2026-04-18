/*
 * sysStat.c
 * State probe that returns FreeRTOS stack high-water marks for all tasks
 * This implementation requires configUSE_TRACE_FACILITY to be enabled so
 * uxTaskGetSystemState can enumerate all tasks.
 */

#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "stateProbe.h"
#include "sysStat.h"

static const char TAG[] = "sysStat";
static stateProbe_probe sysStat_probe;

/* Probe token exposed via MQTT/prov: device_uname/sysStat */
#define SYSSTAT_TOKEN "sysStat"

/* Maximum size for a single stateProbe_write; match other probes' conservative limits */
#define SYSSTAT_OUT_BUF 256

/* Callback invoked when sysStat is requested */
static void sysStat_cb(const char* data, int length)
{
    (void)data; (void)length;

#if !( configUSE_TRACE_FACILITY == 1 )
    /* Guard compile-time: we require trace facility */
    /* Enable it using make menuconfig -> Component Config -> freeRTOS -> Enable FreeRTOS trace facility */
    const char *msg = "sysStat: configUSE_TRACE_FACILITY not enabled\n";
    stateProbe_write(&sysStat_probe, (char*)msg, strlen(msg));
    ESP_LOGW(TAG, "%s", msg);
    return;
#else

    UBaseType_t taskCount = uxTaskGetNumberOfTasks();
    if (taskCount == 0) {
        const char *msg = "sysStat: no tasks\n";
        stateProbe_write(&sysStat_probe, (char*)msg, strlen(msg));
        return;
    }

    TaskStatus_t *taskStatusArray = pvPortMalloc(sizeof(TaskStatus_t) * taskCount);
    if (taskStatusArray == NULL) {
        ESP_LOGE(TAG, "pvPortMalloc failed");
        const char *msg = "sysStat: memfail\n";
        stateProbe_write(&sysStat_probe, (char*)msg, strlen(msg));
        return;
    }

    /* uxTaskGetSystemState returns the number of entries filled */
    UBaseType_t returned = uxTaskGetSystemState(taskStatusArray, taskCount, NULL);

    char outbuf[SYSSTAT_OUT_BUF];
    int idx = 0;
    for (UBaseType_t i = 0; i < returned; i++) {
        int n = snprintf(&outbuf[idx], sizeof(outbuf) - idx, "%s:%u\n",
                         taskStatusArray[i].pcTaskName,
                         (unsigned int)taskStatusArray[i].usStackHighWaterMark);
        if (n < 0) break;
        idx += n;
        if (idx >= (int)sizeof(outbuf) - 1) break;
    }

    vPortFree(taskStatusArray);

    if (idx <= 0) {
        const char *msg = "sysStat: no data\n";
        stateProbe_write(&sysStat_probe, (char*)msg, strlen(msg));
        return;
    }

    outbuf[sizeof(outbuf)-1] = '\0';
    stateProbe_write(&sysStat_probe, outbuf, idx);
#endif
}

void sysStat_init(void)
{
    stateProbe_register(&sysStat_probe, SYSSTAT_TOKEN, sysStat_cb);
}
