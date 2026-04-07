/**
 * @file Board configuration header
 * Contains all board-specific configuration parameters for the Butler device.
 */

#ifndef __BOARD_CFG_H__
#define __BOARD_CFG_H__

#define BOARD_CFG_CHIP ESP8266

#define ACTUATOR_PRIO_NOF 1
#define ACTUATOR_NVCRON_PRIO 0

#define CRON_MAX_ENTRIES  15
#define ENABLE_NVCRON_REMOTE 1

/* Log Ring Buffer Configuration */
#define NVLOGRING_MAX_ENTRIES 32


/* GPIO for Heartbeat indicotor LED: Active low */
#define INDICATOR_LED  2

/* Vulnerability: Connection to unsecure broker, ntp server*/
#define BOARD_CFG_DEFAULT_URL "matrix.local"

#define BOARD_CFG_WIFI_RETRY_DELAY        10000

#define I2C_MASTER_SCL_IO           12               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO           14               /*!< gpio number for I2C master data  */

/* Undefine BOARD_CFG_USE_DS1307 if you want to reclaim the SCL and SDA pins for something else. If this config is enabled
   We will see if a DS1307 is available on the SCL and SDA lines defined above. And use its time if one is found. If not found,
   then get time from ntp, once connectivity is available */
#define BOARD_CFG_USE_DS1307

#define BOARD_CFG_HB_OFF_TIME 3000

#define BOARD_CFG_PIN_NVS_RESET 0

#define BOARD_CFG_URL_SIZE 64 /* Each config string is stored as a fixed-size 64-byte blob in NVS. */


/* PIN Planning
 * GPIO 0: Input for NVS Reset
 * GPIO 1: UART
 * GPIO 2: Heartbeat LED
 * GPIO 3: UART
 * GPIO 4: Actuator 1
 * GPIO 5: Actuator 0
 * GPIO 6: FLASH
 * GPIO 7: FLASH
 * GPIO 8: FLASH
 * GPIO 9: FLASH
 * GPIO 10: FLASH
 * GPIO 11: FLASH
 * GPIO 12: I2C SCL
 * GPIO 13: Actuator 2
 * GPIO 14: I2C SDA
 * GPIO 15: Actuator 3
 * GPIO 16: Not Used
 */

/* Public getters for configurable strings. Each reads a fixed 64-byte NVS
 * blob (exactly 64 bytes) from namespace "board_cfg" and copies at most
 * (len-1) bytes into the provided buffer, ensuring NUL-termination. Returns
 * the length of the string (excluding terminating NUL). On any NVS error
 * the implementation uses ESP_ERROR_CHECK() (no custom error handling).
 */
int boardCfg_get_mqttBroker(char* name, int len);
int boardCfg_get_ntpServer(char* name, int len);
int boardCfg_get_otaHost(char* name, int len);

/* Provisioning/protocomm-style write handler. This has the same signature as
 * the other *_wr handlers in the project and accepts a packed buffer of three
 * null-terminated strings in order: mqttBroker\0 ntpServer\0 otaHost\0.
 * If a particular string is empty (first byte == '\0') the corresponding
 * NVS key will be erased. The handler returns an esp_err_t as usual.
 */
#include <stdint.h>
#include <sys/types.h>
#include <esp_err.h>
esp_err_t boardCfg_wr_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                             uint8_t **outbuf, ssize_t *outlen, void *priv_data);



/* TASK PRIORITIES */
#define DS1307_TASK_PRIO      tskIDLE_PRIORITY+1
#define TICK_TASK_PRIO        tskIDLE_PRIORITY+1
#define LATE_INIT_TASK_PRIO   tskIDLE_PRIORITY+2



/* STACK ALLOTMENT */
#define DS1307_TASK_STACK_SIZE      2048
#define TICK_TASK_STACK_SIZE        2048
#define LATE_INIT_TASK_STACK_SIZE   2048
#endif /*__BOARD_CFG_H__*/
