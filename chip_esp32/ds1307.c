/*
 * esp32/ds1307.c
 * ESP32-specific implementation of the DS1307 RTC interface.
 * Mirrors the behaviour of main/ds1307.c but targets esp-idf's I2C API.
 */

#include <time.h>
#include <esp_log.h>
#include <driver/i2c_master.h>
#include "board_cfg.h"
#include "ds1307.h"

#include <stddef.h>

static const char *TAG = "ds1307";

/* master bus and device handles for new i2c_master API */
static i2c_master_bus_handle_t i2c_bus = NULL;
static i2c_master_dev_handle_t i2c_dev = NULL;

#define DS1307                  0xd0
#define DS1307_SEC_ADDR         0x00
#define DS1307_MIN_ADDR         0x01
#define DS1307_HR_ADDR          0x02
#define DS1307_DATE_ADDR        0x04
#define DS1307_MONTH_ADDR       0x05
#define DS1307_DAY_ADDR         0x03
#define DS1307_YEAR_ADDR        0x06
#define DS1307_CONTROL_ADDR     0x07
#define BCD_2_DECIMAL(x)        ((x >> 4) * 10 + (x & 0x0f))
#define DECIMAL_2_BCD(x)        (((x/10) << 4) | (x%10))

static void ds1307_set(unsigned char address, unsigned char data)
{
    if (i2c_dev == NULL) {
        ESP_LOGW(TAG, "ds1307_set: i2c device not initialized");
        return;
    }

    uint8_t out[2] = { address, data };
    esp_err_t ret = i2c_master_transmit(i2c_dev, out, sizeof(out), 1000);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ds1307_set: i2c_master_transmit returned %d", ret);
    }
}

void ds1307_init(void)
{
    if (i2c_bus != NULL && i2c_dev != NULL) {
        /* already initialized */
        return;
    }

    i2c_master_bus_config_t bus_cfg;
    bus_cfg.i2c_port = I2C_NUM_0;
    /* Use board-configured SDA/SCL pins */
    bus_cfg.sda_io_num = I2C_MASTER_SDA_IO;
    bus_cfg.scl_io_num = I2C_MASTER_SCL_IO;
    bus_cfg.glitch_ignore_cnt = 0;
    bus_cfg.intr_priority = 0;
    bus_cfg.trans_queue_depth = 0;
    bus_cfg.flags.enable_internal_pullup = 1;
    bus_cfg.flags.allow_pd = 0;

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &i2c_bus);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "i2c_new_master_bus failed: %d", err);
        return;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = (DS1307 >> 1),
        .scl_speed_hz = 100000,
        .scl_wait_us = 0,
        .flags.disable_ack_check = 0,
    };

    err = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &i2c_dev);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "i2c_master_bus_add_device failed: %d", err);
        i2c_del_master_bus(i2c_bus);
        i2c_bus = NULL;
        return;
    }

    /* Disable square wave output */
    ds1307_set(DS1307_CONTROL_ADDR, 0x00);
}

esp_err_t ds1307_get_time(struct tm *tm)
{
    if (i2c_dev == NULL) {
        ESP_LOGW(TAG, "ds1307_get_time: i2c device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t start_addr = 0x00;
    uint8_t registers[7] = {0};
    esp_err_t ret = i2c_master_transmit_receive(i2c_dev, &start_addr, 1, registers, sizeof(registers), 1000);
    if (ret != ESP_OK) return ret;

    /*Remove non-time bits*/
    registers[DS1307_SEC_ADDR] &= 0x7f;
    registers[DS1307_HR_ADDR] &= 0x3f;

    /*Form struct tm*/
    tm->tm_hour = BCD_2_DECIMAL(registers[DS1307_HR_ADDR]);
    tm->tm_isdst = 0;
    tm->tm_mday = BCD_2_DECIMAL(registers[DS1307_DATE_ADDR]);
    tm->tm_min = BCD_2_DECIMAL(registers[DS1307_MIN_ADDR]);
    tm->tm_mon = BCD_2_DECIMAL(registers[DS1307_MONTH_ADDR]);
    tm->tm_sec = BCD_2_DECIMAL(registers[DS1307_SEC_ADDR]);
    tm->tm_wday = BCD_2_DECIMAL(registers[DS1307_DAY_ADDR]);
    tm->tm_yday = 0;
    tm->tm_year = BCD_2_DECIMAL(registers[DS1307_YEAR_ADDR]);

    /*Unit Conversion*/
    tm->tm_year += 100;
    tm->tm_mon -= 1;
    tm->tm_wday -= 1;
    return ESP_OK;
}

esp_err_t ds1307_set_time(struct tm *tm)
{
    if (i2c_dev == NULL) {
        ESP_LOGW(TAG, "ds1307_set_time: i2c device not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    while (tm->tm_year > 99)
        tm->tm_year -= 100;
    tm->tm_mon += 1;
    tm->tm_wday += 1;

    uint8_t out[8];
    /* first byte is the start register */
    out[0] = 0x00;
    out[DS1307_SEC_ADDR + 1] = DECIMAL_2_BCD(tm->tm_sec);
    out[DS1307_MIN_ADDR + 1] = DECIMAL_2_BCD(tm->tm_min);
    out[DS1307_HR_ADDR + 1] = DECIMAL_2_BCD(tm->tm_hour) & 0x3f; /* 24 hour mode */
    out[DS1307_DAY_ADDR + 1] = DECIMAL_2_BCD(tm->tm_wday);
    out[DS1307_DATE_ADDR + 1] = DECIMAL_2_BCD(tm->tm_mday);
    out[DS1307_MONTH_ADDR + 1] = DECIMAL_2_BCD(tm->tm_mon);
    out[DS1307_YEAR_ADDR + 1] = DECIMAL_2_BCD(tm->tm_year);

    /* Clear CH bit in seconds */
    out[DS1307_SEC_ADDR + 1] &= 0x7f;

    esp_err_t ret = i2c_master_transmit(i2c_dev, out, sizeof(out), 1000);
    return ret;
}
