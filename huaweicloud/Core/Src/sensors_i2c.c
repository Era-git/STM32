#include "sensors_i2c.h"
#include <string.h>
#include <stdint.h>

#define SHT30_ADDR (0x44u << 1)
#define BH1750_ADDR (0x23u << 1)

static I2C_HandleTypeDef *s_i2c;
static uint8_t bmp_addr; /* 0x76 or 0x77, HAL format <<1 */

static uint16_t bmp_dig_T1;
static int16_t bmp_dig_T2, bmp_dig_T3;
static uint16_t bmp_dig_P1;
static int16_t bmp_dig_P2, bmp_dig_P3, bmp_dig_P4, bmp_dig_P5;
static int16_t bmp_dig_P6, bmp_dig_P7, bmp_dig_P8, bmp_dig_P9;

static HAL_StatusTypeDef i2c_mem_read(uint16_t dev7, uint8_t reg, uint8_t *buf, uint16_t len)
{
  return HAL_I2C_Mem_Read(s_i2c, dev7, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 80);
}

static HAL_StatusTypeDef i2c_mem_write(uint16_t dev7, uint8_t reg, uint8_t *buf, uint16_t len)
{
  return HAL_I2C_Mem_Write(s_i2c, dev7, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 80);
}

static int bmp280_try_init(uint16_t addr7)
{
  uint8_t id = 0;
  if (i2c_mem_read(addr7, 0xD0, &id, 1) != HAL_OK || id != 0x58u)
    return 0;

  uint8_t cal[24];
  if (i2c_mem_read(addr7, 0x88, cal, 24) != HAL_OK)
    return 0;

  bmp_dig_T1 = (uint16_t)cal[0] | ((uint16_t)cal[1] << 8);
  bmp_dig_T2 = (int16_t)((uint16_t)cal[2] | ((uint16_t)cal[3] << 8));
  bmp_dig_T3 = (int16_t)((uint16_t)cal[4] | ((uint16_t)cal[5] << 8));
  bmp_dig_P1 = (uint16_t)cal[6] | ((uint16_t)cal[7] << 8);
  bmp_dig_P2 = (int16_t)((uint16_t)cal[8] | ((uint16_t)cal[9] << 8));
  bmp_dig_P3 = (int16_t)((uint16_t)cal[10] | ((uint16_t)cal[11] << 8));
  bmp_dig_P4 = (int16_t)((uint16_t)cal[12] | ((uint16_t)cal[13] << 8));
  bmp_dig_P5 = (int16_t)((uint16_t)cal[14] | ((uint16_t)cal[15] << 8));
  bmp_dig_P6 = (int16_t)((uint16_t)cal[16] | ((uint16_t)cal[17] << 8));
  bmp_dig_P7 = (int16_t)((uint16_t)cal[18] | ((uint16_t)cal[19] << 8));
  bmp_dig_P8 = (int16_t)((uint16_t)cal[20] | ((uint16_t)cal[21] << 8));
  bmp_dig_P9 = (int16_t)((uint16_t)cal[22] | ((uint16_t)cal[23] << 8));

  uint8_t cfg = 0x27; /* osrs_t x1, osrs_p x1, mode normal (3) */
  if (i2c_mem_write(addr7, 0xF4, &cfg, 1) != HAL_OK)
    return 0;
  bmp_addr = (uint8_t)addr7;
  return 1;
}

static void bmp280_read(Sensor_I2C_Data_t *out)
{
  out->bmp_ok = 0;
  out->bmp_temp_c10 = 0;
  out->bmp_press_pa = 0;
  if (bmp_addr == 0)
    return;

  uint8_t d[6];
  if (i2c_mem_read(bmp_addr, 0xF7, d, 6) != HAL_OK)
    return;

  int32_t adc_P = (int32_t)(((uint32_t)d[0] << 12) | ((uint32_t)d[1] << 4) | ((uint32_t)d[2] >> 4));
  int32_t adc_T = (int32_t)(((uint32_t)d[3] << 12) | ((uint32_t)d[4] << 4) | ((uint32_t)d[5] >> 4));

  int32_t var1i = ((((adc_T >> 3) - ((int32_t)bmp_dig_T1 << 1)) * (int32_t)bmp_dig_T2) >> 11;
  int32_t var2i = (((((adc_T >> 4) - (int32_t)bmp_dig_T1) * ((adc_T >> 4) - (int32_t)bmp_dig_T1)) >> 12) *
                   (int32_t)bmp_dig_T3) >>
                  14;
  int32_t t_fine = var1i + var2i;
  out->bmp_temp_c10 = (int16_t)(((int64_t)t_fine * 5 + 128) * 10 / 25600);

  int64_t var1 = (int64_t)t_fine - 128000;
  int64_t var2 = var1 * var1 * (int64_t)bmp_dig_P6;
  var2 = var2 + ((var1 * (int64_t)bmp_dig_P5) << 17);
  var2 = var2 + ((int64_t)bmp_dig_P4 << 35);
  var1 = ((var1 * var1 * (int64_t)bmp_dig_P3) >> 8) + ((var1 * (int64_t)bmp_dig_P2) << 12);
  var1 = (((((int64_t)1 << 47) + var1)) * (int64_t)bmp_dig_P1) >> 33;
  if (var1 == 0)
    return;

  int64_t p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = ((int64_t)bmp_dig_P9 * (p >> 13) * (p >> 13)) >> 25;
  var2 = ((int64_t)bmp_dig_P8 * p) >> 19;
  p = (p + var1 + var2) >> 4;
  p += (int64_t)bmp_dig_P7 << 4;
  if (p < 0)
    p = 0;
  if (p > 2147483647LL)
    p = 2147483647LL;
  out->bmp_press_pa = (int32_t)p;
  out->bmp_ok = 1;
}

static void sht30_read(Sensor_I2C_Data_t *out)
{
  out->sht_ok = 0;
  out->sht_temp_c10 = 0x7FFF;
  out->sht_rh10 = 0;
  uint8_t cmd[2] = {0x24, 0x00};
  if (HAL_I2C_Master_Transmit(s_i2c, SHT30_ADDR, cmd, 2, 50) != HAL_OK)
    return;
  HAL_Delay(20);
  uint8_t r[6];
  if (HAL_I2C_Master_Receive(s_i2c, SHT30_ADDR, r, 6, 50) != HAL_OK)
    return;

  uint16_t st = ((uint16_t)r[0] << 8) | r[1];
  uint16_t srh = ((uint16_t)r[3] << 8) | r[4];
  int32_t t1000 = -4500 + (17500 * (int32_t)st) / 65535;
  int32_t rh1000 = (10000 * (int32_t)srh) / 65535;
  if (rh1000 > 10000)
    rh1000 = 10000;
  if (rh1000 < 0)
    rh1000 = 0;
  out->sht_temp_c10 = (int16_t)((t1000 + 5) / 10);
  out->sht_rh10 = (int16_t)((rh1000 + 5) / 10);
  out->sht_ok = 1;
}

static uint8_t bh_inited;

static void bh1750_read(Sensor_I2C_Data_t *out)
{
  out->bh_ok = 0;
  out->bh_lux = 0xFFFF;
  if (!bh_inited)
  {
    uint8_t on = 0x01;
    if (HAL_I2C_Master_Transmit(s_i2c, BH1750_ADDR, &on, 1, 20) != HAL_OK)
      return;
    uint8_t cont = 0x10;
    if (HAL_I2C_Master_Transmit(s_i2c, BH1750_ADDR, &cont, 1, 20) != HAL_OK)
      return;
    bh_inited = 1;
    HAL_Delay(120);
  }

  uint8_t raw[2];
  if (HAL_I2C_Master_Receive(s_i2c, BH1750_ADDR, raw, 2, 80) != HAL_OK)
    return;
  uint16_t v = ((uint16_t)raw[0] << 8) | raw[1];
  uint32_t lx = ((uint32_t)v * 10u) / 12u;
  if (lx > 65535u)
    lx = 65535u;
  out->bh_lux = (uint16_t)lx;
  out->bh_ok = 1;
}

void Sensors_Init(I2C_HandleTypeDef *hi2c)
{
  s_i2c = hi2c;
  bmp_addr = 0;
  bh_inited = 0;
  if (bmp280_try_init(0x76u << 1))
    return;
  (void)bmp280_try_init(0x77u << 1);
}

void Sensors_Read(Sensor_I2C_Data_t *out)
{
  memset(out, 0, sizeof(*out));
  out->sht_temp_c10 = 0x7FFF;
  out->bh_lux = 0xFFFF;
  sht30_read(out);
  bmp280_read(out);
  bh1750_read(out);
}
