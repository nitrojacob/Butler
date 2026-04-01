/**
 * @file Ring Buffer Logging implementation on top of NVS flash of Espressif microcontrollers.
 * Log persists across power cycles. NVS has built in wear levelling and data integrity during accidental power fails.
 * The supervisor can pull the log buffer from the host side.
 */

#ifndef __NVLOGRING_H__
#define __NVLOGRING_H__

#include <stdint.h>
#include <esp_err.h>
#include "fixedTypes.h"


#define NVLOGRING_MAX_ENTRY_LENGTH 255

/* Endpoint names for stateProbe and wifi_prov_mgr */
#define NVLOGRING_PROBE_ENDPOINT "plogRd"

/* Log entry structure */
typedef struct {
    char log[NVLOGRING_MAX_ENTRY_LENGTH + 1];    /* Last byte should be null */
} s_nvLogRing_entry;

/* Public API */
/* Write a single log entry. Must be null terminated string. */
void nvLogRing_write(const char* log);

/* Read the log buffer */
int nvLogRing_read(uint8_t* buffer, int length);

/* Initialize the library. To be called early during boot, preferably immediately after nvs_init()
 * Enables the driver to accept and log messages. Attempting to write before nvLogRing_init() is 
 * called will fail silently, and the log entry will be lost.
 */
void nvLogRing_init(void);

#endif /* __NVLOGRING_H__ */
