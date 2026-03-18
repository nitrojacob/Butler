/**
 * @file Cron implementation on top of onchip eeprom. All crontab entries stored in on-chip eeprom, so that it can resume operation
 * after a power failure independently, even if there is no supervisor on the network to instruct it with corntab instructions.
 * The supervisor can update the crontab information over a network, and it will be stored persistently.
 */

#ifndef __NVCRON_H__
#define __NVCRON_H__
#include "fixedTypes.h"

typedef enum{
  CRON_FUNC_RELAY_OFF  = 0,
  CRON_FUNC_RELAY_ON,
  CRON_FUNC_NOF,
  CRON_FUNC_EMPTY     = 0xff,
}e_cronFunc;

void nvCron_init(U8 hour, U8 minute);
void nvCron_tick(U8 hour, U8 minute);
void nvCron_addEntry(U8 hour, U8 minute, e_cronFunc func, U8 arg);
void nvCron_delEntry(U8 hour, U8 minute, e_cronFunc func, U8 arg);

/**
 * @brief Reads all cron entries from NVS and writes them to the provided buffer. The buffer should be large enough to hold CRON_MAX_ENTRIES entries.
 * @param buffer Pointer to the buffer where the cron entries will be written. The entries will be written in the same format as the s_cronEntry structure defined in nvCron.c.
 * @param length Length of the buffer in bytes. Should ideally be CRON_MAX_ENTRIES * sizeof(s_cronEntry). If smaller, only the number of entries that can fit in the buffer will be read and written.
 * @return The number of bytes written to the buffer, or a negative value if an error occurred. The entries will be in the same format as the s_cronEntry structure defined in nvCron.c.
 */
int nvCron_readMultipleEntry(char* buffer, int length);

/**
 * @brief Writes multiple cron entries to NVS from the provided buffer. The buffer should contain CRON_MAX_ENTRIES entries in the same format as the s_cronEntry structure defined in nvCron.c.
 * @param buffer Pointer to the buffer containing the cron entries to be written. The entries should be in the same format as the s_cronEntry structure defined in nvCron.c.
 * @param length Length of the buffer in bytes. Should be at least CRON_MAX_ENTRIES * sizeof(s_cronEntry).
 */
void nvCron_writeMultipleEntry(const char* buffer, int length);

void nvCronRemote_init(void);

#endif /*__NVCRON_H__*/
