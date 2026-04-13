/* Simple stubs for ESP-IDF functions and project-specific helpers used by nvCron.c
 * These are only used in unit tests and delegate actuator functions to the actuator mock
 * and provide minimal implementations for logging/stateProbe functions.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "actuator_mock.h"
#include "stateProbe.h"
#include "esp_err.h"

/* Map actuator calls used by nvCron.c to the actuator mock helpers */
void actuator_on(unsigned int actuator, unsigned int priority)
{
    (void)priority;
    actuator_mock_set_on_called(true);
}

void actuator_off(unsigned int actuator, unsigned int priority)
{
    (void)priority;
    actuator_mock_set_off_called(true);
}

void actuator_reflectState(void)
{
    actuator_mock_set_reflectState_called(true);
}

/* Minimal trap logging function used in production code */
void trap_butler_log(const char* msg)
{
    (void)msg; /* Discard in unit tests */
}

/* Minimal esp-error helpers expected by ESP_ERROR_CHECK macro in production */
void _esp_error_check_failed(esp_err_t rc, const char* file, int line, const char* function, const char* expression)
{
    /* In unit tests, print and abort to make failures visible */
    fprintf(stderr, "ESP_ERROR_CHECK failed: rc=%d at %s:%d (%s) expr=%s\n", rc, file, line, function, expression);
    abort();
}

/* Minimal logging stubs */
int esp_log_write(int level, const char* tag, const char* format, ...)
{
    (void)level; (void)tag; (void)format; return 0;
}

const char* esp_err_to_name(esp_err_t err)
{
    if (err == ESP_OK) return "ESP_OK";
    return "ESP_ERR_UNKNOWN";
}

/* Minimal stateProbe stubs used by nvCron.c */
void stateProbe_register(stateProbe_probe* pProbe, const char* probeSymbol, probeFn pProbeFunction)
{
    (void)pProbe; (void)probeSymbol; (void)pProbeFunction;
}

void stateProbe_write(stateProbe_probe* probe, char* buf, int len)
{
    (void)probe; (void)buf; (void)len;
}

/* Minimal stateProbe functions to satisfy link; implementations intentionally empty */
void stateProbe_init(const char* device_uname, unsigned char len_device_uname) { (void)device_uname; (void)len_device_uname; }
void stateProbe_startTask(void) {}
void stateProbe_late_init(void) {}
void stateProbe_log(char* log) { (void)log; }
