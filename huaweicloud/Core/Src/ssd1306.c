#include "ssd1306.h"
#include <string.h>

#define SSD1306_I2C_ADDR (0x3Cu << 1)
#define SSD1306_W 128
#define SSD1306_H 64
#define SSD1306_PAGES (SSD1306_H / 8)

extern const uint8_t font5x7_ascii[];

static I2C_HandleTypeDef *ssd_hi2c;
static uint8_t ssd_fb[SSD1306_W * SSD1306_PAGES];

static void ssd_cmd(uint8_t c)
{
  uint8_t b[2] = {0x00, c};
  HAL_I2C_Master_Transmit(ssd_hi2c, SSD1306_I2C_ADDR, b, 2, 50);
}

static void ssd_cmds(const uint8_t *d, unsigned n)
{
  uint8_t buf[8];
  unsigned i = 0;
  while (i < n)
  {
    unsigned chunk = n - i;
    if (chunk > sizeof(buf) - 1u)
      chunk = sizeof(buf) - 1u;
    buf[0] = 0x00;
    memcpy(buf + 1, d + i, chunk);
    HAL_I2C_Master_Transmit(ssd_hi2c, SSD1306_I2C_ADDR, buf, chunk + 1u, 50);
    i += chunk;
  }
}

void SSD1306_Init(I2C_HandleTypeDef *hi2c)
{
  ssd_hi2c = hi2c;
  HAL_Delay(20);

  const uint8_t init_seq[] = {
      0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14,
      0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1,
      0xDB, 0x40, 0xA4, 0xA6, 0xAF};
  ssd_cmds(init_seq, sizeof(init_seq));
  SSD1306_Clear();
  SSD1306_Refresh();
}

void SSD1306_Clear(void)
{
  memset(ssd_fb, 0, sizeof(ssd_fb));
}

void SSD1306_Refresh(void)
{
  const uint8_t pre[] = {0x21, 0x00, 0x7F, 0x22, 0x00, 0x07};
  ssd_cmds(pre, sizeof(pre));

  for (uint8_t page = 0; page < SSD1306_PAGES; page++)
  {
    uint8_t chunk[17];
    chunk[0] = 0x40;
    for (uint8_t col = 0; col < SSD1306_W; col += 16)
    {
      memcpy(chunk + 1, &ssd_fb[(uint16_t)page * SSD1306_W + col], 16);
      HAL_I2C_Master_Transmit(ssd_hi2c, SSD1306_I2C_ADDR, chunk, 17, 100);
    }
  }
}

static void ssd_pixel_on(uint8_t x, uint8_t y)
{
  if (x >= SSD1306_W || y >= SSD1306_H)
    return;
  ssd_fb[(y / 8) * SSD1306_W + x] |= (uint8_t)(1u << (y & 7u));
}

void SSD1306_DrawChar(uint8_t x, uint8_t y, char c)
{
  if (c < 32 || c > 127)
    c = ' ';
  const uint8_t *glyph = &font5x7_ascii[(unsigned)(c - 32) * 5u];
  for (uint8_t i = 0; i < 5u; i++)
  {
    uint8_t line = glyph[i];
    for (uint8_t j = 0; j < 7u; j++, line >>= 1u)
    {
      if (line & 1u)
        ssd_pixel_on((uint8_t)(x + i), (uint8_t)(y + j));
    }
  }
}

void SSD1306_DrawStr(uint8_t x, uint8_t y, const char *s)
{
  while (*s)
  {
    if (x > SSD1306_W - 6u)
      break;
    SSD1306_DrawChar(x, y, *s++);
    x += 6u;
  }
}
