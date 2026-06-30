#ifndef SSD1306_H
#define SSD1306_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

void SSD1306_Init(I2C_HandleTypeDef *hi2c);
void SSD1306_Clear(void);
void SSD1306_Refresh(void);
void SSD1306_DrawChar(uint8_t x, uint8_t y, char c);
void SSD1306_DrawStr(uint8_t x, uint8_t y, const char *s);

#endif
