/**
 * @file Ring Buffer Logging implementation on top of NVS flash of Espressif microcontrollers.
 * Log persists across power cycles. NVS has built in wear levelling and data integrity during accidental power fails.
 * The supervisor can pull the log buffer from the host side.
 * 
 * WORKING:
 * 
 *            +-----+      +-----+      +-----+      +-----+      +-----+      +-----+
 * LOG ENTRY: | n-2 |      | n-1 |      |  n  |      |     |      | n-4 |      | n-3 |
 *            +-----+      +-----+      +-----+      +-----+      +-----+      +-----+
 * NVS INDEX:    0            1            2            3            4            5
 * 
 * We need to minimize the number of writes to NVS with each log entry Addition. We don't want to
 * maintain separate HEAD or TAIL pointer that needs to be updated with every log entry addition.
 * Shown Above is the case for NVLOGRING_MAX_ENTRIES = 5. 5 active log entires and 6 index used in that case.
 * One of the index will be always be blank. HEAD and TAIL can be identified based on the BLANK Entry.
 * The BLANK Entry is where you always write next.
 * At boot find out the HEAD by traversal. (search for ESP_ERR_NVS_NOT_FOUND)
 * Every time a new entry is added, increment HEAD, delete (HEAD+1)%(NVLOGRING_MAX_ENTRIES+1), write the entry.
 * This way we are guaranteed to have alteast 1 blank entry, even if there is power failure between delete and write.
 * 
 * To read all entries, start from HEAD and read until you find a blank entry. This will give you all the entries in order from most recent to least recent.
 */

/*-------------------- INCLUDES --------------------*/
#include <string.h>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <nvs.h>
#include <esp_log.h>
#include <esp_err.h>

#include "stateProbe.h"
#include "board_cfg.h"
#include "nvLogRing.h"

/*-------------------- DEFINES --------------------*/
/* NVS namespace and keys */
#define NVLOGRING_NVS_NAMESPACE "nvLogRing"
#define NVLOGRING_NVS_KEY_ENTRY "log"

#define INCREMENT_POINTER(pointer) do { \
        (pointer)++; \
        if ((pointer) >= (NVLOGRING_MAX_ENTRIES+1)) { \
            (pointer) -= (NVLOGRING_MAX_ENTRIES+1); \
        } \
    } while(0)
#define DECREMENT_POINTER(pointer) do { \
        if ((pointer) <= 0) { \
            (pointer) += (NVLOGRING_MAX_ENTRIES+1); \
        } \
        (pointer)--; \
    } while(0)

/*-------------------- LOCAL VARIABLES --------------------*/
static nvs_handle_t nvs = (nvs_handle_t)0;
static S16 head = 0, current = 0;
/* StateProbe probe definition */
static stateProbe_probe plogRd_probe;
//static const char TAG[] = "nvLogRing";


/*-------------------- LOCAL FUNCTIONS --------------------*/

/* StateProbe endpoint handler */
static void nvLogRing_probe_cb(const char* buffer, int length) {
    char read_buffer[NVLOGRING_MAX_ENTRY_LENGTH + 1];
    int bytes_read = nvLogRing_read((unsigned char*)read_buffer, NVLOGRING_MAX_ENTRY_LENGTH + 1);
    stateProbe_write(&plogRd_probe, read_buffer, bytes_read);
}

/*-------------------- GLOBAL FUNCTIONS --------------------*/
/* Write a log entry to the ring buffer. Silently fail in case of errors.
   This avoids recursive failure in case logging an NVS failure itself */
void nvLogRing_write(const char* log) {
    char key[16];
    s_nvLogRing_entry entry;
    S16 free_block;
    int data_size;
    if (!nvs || !log) {
        return;
    }
    data_size = strlen(log);
    if (data_size > NVLOGRING_MAX_ENTRY_LENGTH) {
        data_size = NVLOGRING_MAX_ENTRY_LENGTH;
    }
    memcpy(entry.log, log, data_size);
    entry.log[data_size] = '\0';

    free_block = head;
    /* Advance head pointer */
    INCREMENT_POINTER(head);

    /* Advance the Empty Slot first */
    snprintf(key, sizeof(key), "%s%d", NVLOGRING_NVS_KEY_ENTRY, head);
    nvs_erase_key(nvs, key);
    nvs_commit(nvs);

    /* Write to NVS */
    snprintf(key, sizeof(key), "%s%d", NVLOGRING_NVS_KEY_ENTRY, free_block);
    nvs_set_blob(nvs, key, &entry, sizeof(s_nvLogRing_entry));
    nvs_commit(nvs);
}

/* Read one log entry from the ring buffer. Which log entry is kept track using variable current.
 * Blank entry will read as "-". strlen = 1. Just the '-' followed by null character!
 * Direction of read is tail to head. So last read before the blank is the head/newest message.
 * The one after the blank is the tail. The client can read NVLOGRING_MAX_ENTRIES+1 number of times
 * to get the entire log buffer and then reorder and reconstruct the log.
 */
int nvLogRing_read(uint8_t* buffer, int length) {
    char key[16];
    int bytes_written = 0;
    s_nvLogRing_entry entry;
    size_t entry_size;

    if (!nvs || !buffer || length < 2) {
        return 0;
    }
    entry_size = sizeof(s_nvLogRing_entry);
    snprintf(key, sizeof(key), "%s%d", NVLOGRING_NVS_KEY_ENTRY, current);
    esp_err_t err = nvs_get_blob(nvs, key, &entry, &entry_size);
    if (err == ESP_ERR_NOT_FOUND) {
        buffer[0] = '-';
        buffer[1] = '\0';
        bytes_written = 2;
    }
    else if (err == ESP_OK)
    {
        bytes_written = strlen(entry.log) + 1; /* strleng gives length excluding null termination */
        if (length < bytes_written)
            bytes_written = length;
        memcpy(buffer, &entry.log, bytes_written);
        buffer[bytes_written - 1] = '\0';
    }
    else{
        /*Silent Failure*/
        buffer[0] = '?';
        buffer[1] = '\0';
        bytes_written = 2;
    }
    INCREMENT_POINTER(current);
    return bytes_written;
}

/* Initialize NVS and ring buffer state */
void nvLogRing_init(void) {
    char key[16];
    s_nvLogRing_entry entry;
    ESP_ERROR_CHECK( nvs_open(NVLOGRING_NVS_NAMESPACE, NVS_READWRITE, &nvs) );

    /* Find head by traversing and finding first blank entry */
    for (head = 0; head < NVLOGRING_MAX_ENTRIES+1; head++) {
        snprintf(key, sizeof(key), "%s%d", NVLOGRING_NVS_KEY_ENTRY, head);
        size_t size = sizeof(s_nvLogRing_entry);
        esp_err_t err = nvs_get_blob(nvs, key, &entry, &size);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            break;
        }
    }
    /* HEAD always points the blank entry */
}

/* Initialize StateProbe endpoint */
void nvLogRing_stateProbe_init(void) {
    stateProbe_register(&plogRd_probe, NVLOGRING_PROBE_ENDPOINT, nvLogRing_probe_cb);
}
