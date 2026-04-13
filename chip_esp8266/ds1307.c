#include <time.h>
#include <esp_system.h>

#include <driver/i2c.h>

#include "board_cfg.h"
#include "ds1307.h"

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

/*
static unsigned char ds1307_get(unsigned char address)
{
  unsigned char result;
  unsigned char buff[] = {DS1307|0x01, address};
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  i2c_master_start(cmd);
  i2c_master_write(cmd, buff, 2, 1);
  i2c_master_stop(cmd);
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, DS1307|0x01, 1);
  i2c_master_read_byte(cmd, &result, I2C_MASTER_NACK);
  i2c_master_stop(cmd);

  i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);

  return result;
}*/

static void ds1307_set(unsigned char address, unsigned char data)
{
  unsigned char buff[] = {DS1307, address, data};
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  i2c_master_start(cmd);
  i2c_master_write(cmd, buff, sizeof(buff), 1);
  i2c_master_stop(cmd);

  i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
}

void ds1307_init(void)
{
  //unsigned char sec;
  //unsigned char hr;
  i2c_config_t i2c_cfg;
  ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER));
  i2c_cfg.mode = I2C_MODE_MASTER;
  i2c_cfg.scl_io_num = I2C_MASTER_SCL_IO;
  i2c_cfg.scl_pullup_en = 1;
  i2c_cfg.sda_io_num = I2C_MASTER_SDA_IO;
  i2c_cfg.sda_pullup_en = 1;
  i2c_cfg.clk_stretch_tick = 300;
  ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_cfg));


  ds1307_set(DS1307_CONTROL_ADDR,0x00);       /* Disabling the square wave output */
  //sec = ds1307_get(DS1307_SEC_ADDR);
  //if(sec & 0x80)
  //  ds1307_set(DS1307_SEC_ADDR, sec & 0x7f);  /* Clearing the clock Halt bit */
  //hr = ds1307_get(DS1307_HR_ADDR);
  //if((hr&0x40) != 0)
  //  ds1307_set(DS1307_HR_ADDR, hr&0xbf);      /* 24 hr clock mode */
}

esp_err_t ds1307_get_time(struct tm *tm)
{
  unsigned char buff[] = {DS1307, 0};
  unsigned char registers[7];
  esp_err_t ret;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  i2c_master_start(cmd);
  i2c_master_write(cmd, buff, sizeof(buff), 1);
  i2c_master_stop(cmd);
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, DS1307|0x01, 1);
  i2c_master_read(cmd, registers, sizeof(registers), I2C_MASTER_LAST_NACK);
  i2c_master_stop(cmd);

  ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);

  /*Debug Print
  for(int i = 0; i<sizeof(registers); i++)
    printf("DS1307_reg[%d]:%x\n", i, registers[i]);*/

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
  return ret;
}

esp_err_t ds1307_set_time(struct tm *tm)
{
  unsigned char buff[] = {DS1307, 0, 0, 0, 0, 0, 0, 0, 0};
  unsigned char* registers = &(buff[2]);
  esp_err_t ret;

  while(tm->tm_year > 99)
    tm->tm_year -= 100;
  tm->tm_mon += 1;
  tm->tm_wday += 1;
  /*Form register values from struct tm*/
  registers[DS1307_HR_ADDR] = DECIMAL_2_BCD(tm->tm_hour);

  registers[DS1307_DAY_ADDR] = DECIMAL_2_BCD(tm->tm_wday);
  registers[DS1307_MIN_ADDR] = DECIMAL_2_BCD(tm->tm_min);
  registers[DS1307_MONTH_ADDR] = DECIMAL_2_BCD(tm->tm_mon);
  registers[DS1307_SEC_ADDR] = DECIMAL_2_BCD(tm->tm_sec);
  registers[DS1307_YEAR_ADDR] = DECIMAL_2_BCD(tm->tm_year);
  registers[DS1307_DATE_ADDR] = DECIMAL_2_BCD(tm->tm_mday);

  /*Debug Print*/
  /*printf("Year:%d, reg:%x\n", tm->tm_year, registers[DS1307_YEAR_ADDR]);
  printf("Month:%d, reg:%x\n", tm->tm_mon, registers[DS1307_MONTH_ADDR]);
  printf("Date:%d, reg:%x\n", tm->tm_mday, registers[DS1307_DATE_ADDR]);
  printf("Hour:%d, reg:%x\n", tm->tm_hour, registers[DS1307_HR_ADDR]);
  printf("Min:%d, reg:%x\n", tm->tm_min, registers[DS1307_MIN_ADDR]);
  printf("Sec:%d, reg:%x\n", tm->tm_sec, registers[DS1307_SEC_ADDR]);*/

  /*Add non-time bits*/
  registers[DS1307_HR_ADDR] &= 0x3f;   /* 24 hour mode */
  registers[DS1307_SEC_ADDR] &= 0x7f;  /* Clear CH bit */

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  i2c_master_start(cmd);
  i2c_master_write(cmd, buff, sizeof(buff), true);
  i2c_master_stop(cmd);

  ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);

  return ret;
}

