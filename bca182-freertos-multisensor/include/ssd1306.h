#pragma once

#include <stdint.h>

bool SSD1306_Init(void);
void SSD1306_Clear(void);
void SSD1306_DrawText(uint8_t x, uint8_t page, const char *text);
bool SSD1306_Update(void);
bool SSD1306_DisplayOff(void);

