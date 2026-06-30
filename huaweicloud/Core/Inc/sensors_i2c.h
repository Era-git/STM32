#ifndef SENSORS_I2C_H
#define SENSORS_I2C_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

typedef struct {
  int16_t sht_temp_c10;   /* SHT30 temp * 10, 0x7FFF = invalid */
  int16_t sht_rh10;       /* RH * 10 */
  int16_t bmp_temp_c10;
  int32_t bmp_press_pa;   /* 0 = invalid */
  uint16_t bh_lux;        /* 0xFFFF = invalid */
  uint8_t sht_ok;
  uint8_t bmp_ok;
  uint8_t bh_ok;
} Sensor_I2C_Data_t;

void Sensors_Init(I2C_HandleTypeDef *hi2c);
void Sensors_Read(Sensor_I2C_Data_t *out);

#endif
