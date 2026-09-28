#include "ssd1306.h"

#include <cstring>

#include "stm32f1xx_hal.h"
#include "hardware.h"

namespace {

constexpr uint16_t OLED_ADDRESS = (0x3CU << 1);
uint8_t framebuffer[128 * 8] = {};

bool SendCommand(uint8_t command)
{
    uint8_t packet[2] = {0x00, command};
    return HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDRESS, packet, sizeof(packet), 100) == HAL_OK;
}

void Glyph(char ch, uint8_t out[5])
{
    std::memset(out, 0, 5);


    switch (ch) {
        case 'A': { uint8_t g[5]={0x7E,0x11,0x11,0x11,0x7E}; std::memcpy(out,g,5); break; }
        case 'B': { uint8_t g[5]={0x7F,0x49,0x49,0x49,0x36}; std::memcpy(out,g,5); break; }
        case 'C': { uint8_t g[5]={0x3E,0x41,0x41,0x41,0x22}; std::memcpy(out,g,5); break; }
        case 'D': { uint8_t g[5]={0x7F,0x41,0x41,0x22,0x1C}; std::memcpy(out,g,5); break; }
        case 'E': { uint8_t g[5]={0x7F,0x49,0x49,0x49,0x41}; std::memcpy(out,g,5); break; }
        case 'F': { uint8_t g[5]={0x7F,0x09,0x09,0x09,0x01}; std::memcpy(out,g,5); break; }
        case 'G': { uint8_t g[5]={0x3E,0x41,0x49,0x49,0x7A}; std::memcpy(out,g,5); break; }
        case 'H': { uint8_t g[5]={0x7F,0x08,0x08,0x08,0x7F}; std::memcpy(out,g,5); break; }
        case 'I': { uint8_t g[5]={0x00,0x41,0x7F,0x41,0x00}; std::memcpy(out,g,5); break; }
        case 'J': { uint8_t g[5]={0x20,0x40,0x41,0x3F,0x01}; std::memcpy(out,g,5); break; }
        case 'K': { uint8_t g[5]={0x7F,0x08,0x14,0x22,0x41}; std::memcpy(out,g,5); break; }
        case 'L': { uint8_t g[5]={0x7F,0x40,0x40,0x40,0x40}; std::memcpy(out,g,5); break; }
        case 'M': { uint8_t g[5]={0x7F,0x02,0x0C,0x02,0x7F}; std::memcpy(out,g,5); break; }
        case 'N': { uint8_t g[5]={0x7F,0x04,0x08,0x10,0x7F}; std::memcpy(out,g,5); break; }
        case 'O': { uint8_t g[5]={0x3E,0x41,0x41,0x41,0x3E}; std::memcpy(out,g,5); break; }
        case 'P': { uint8_t g[5]={0x7F,0x09,0x09,0x09,0x06}; std::memcpy(out,g,5); break; }
        case 'Q': { uint8_t g[5]={0x3E,0x41,0x51,0x21,0x5E}; std::memcpy(out,g,5); break; }
        case 'R': { uint8_t g[5]={0x7F,0x09,0x19,0x29,0x46}; std::memcpy(out,g,5); break; }
        case 'S': { uint8_t g[5]={0x46,0x49,0x49,0x49,0x31}; std::memcpy(out,g,5); break; }
        case 'T': { uint8_t g[5]={0x01,0x01,0x7F,0x01,0x01}; std::memcpy(out,g,5); break; }
        case 'U': { uint8_t g[5]={0x3F,0x40,0x40,0x40,0x3F}; std::memcpy(out,g,5); break; }
        case 'V': { uint8_t g[5]={0x1F,0x20,0x40,0x20,0x1F}; std::memcpy(out,g,5); break; }
        case 'W': { uint8_t g[5]={0x7F,0x20,0x18,0x20,0x7F}; std::memcpy(out,g,5); break; }
        case 'X': { uint8_t g[5]={0x63,0x14,0x08,0x14,0x63}; std::memcpy(out,g,5); break; }
        case 'Y': { uint8_t g[5]={0x03,0x04,0x78,0x04,0x03}; std::memcpy(out,g,5); break; }
        case 'Z': { uint8_t g[5]={0x61,0x51,0x49,0x45,0x43}; std::memcpy(out,g,5); break; }
        case '0': { uint8_t g[5]={0x3E,0x51,0x49,0x45,0x3E}; std::memcpy(out,g,5); break; }
        case '1': { uint8_t g[5]={0x00,0x42,0x7F,0x40,0x00}; std::memcpy(out,g,5); break; }
        case '2': { uint8_t g[5]={0x42,0x61,0x51,0x49,0x46}; std::memcpy(out,g,5); break; }
        case '3': { uint8_t g[5]={0x21,0x41,0x45,0x4B,0x31}; std::memcpy(out,g,5); break; }
        case '4': { uint8_t g[5]={0x18,0x14,0x12,0x7F,0x10}; std::memcpy(out,g,5); break; }
        case '5': { uint8_t g[5]={0x27,0x45,0x45,0x45,0x39}; std::memcpy(out,g,5); break; }
        case '6': { uint8_t g[5]={0x3C,0x4A,0x49,0x49,0x30}; std::memcpy(out,g,5); break; }
        case '7': { uint8_t g[5]={0x01,0x71,0x09,0x05,0x03}; std::memcpy(out,g,5); break; }
        case '8': { uint8_t g[5]={0x36,0x49,0x49,0x49,0x36}; std::memcpy(out,g,5); break; }
        case '9': { uint8_t g[5]={0x06,0x49,0x49,0x29,0x1E}; std::memcpy(out,g,5); break; }
        case '-': { uint8_t g[5]={0x08,0x08,0x08,0x08,0x08}; std::memcpy(out,g,5); break; }
        case '.': { uint8_t g[5]={0x00,0x60,0x60,0x00,0x00}; std::memcpy(out,g,5); break; }
        case '%': { uint8_t g[5]={0x63,0x13,0x08,0x64,0x63}; std::memcpy(out,g,5); break; }
        case ':': { uint8_t g[5]={0x00,0x36,0x36,0x00,0x00}; std::memcpy(out,g,5); break; }
        case '/': { uint8_t g[5]={0x20,0x10,0x08,0x04,0x02}; std::memcpy(out,g,5); break; }
        case 'e': { uint8_t g[5]={0x38,0x54,0x54,0x54,0x18}; std::memcpy(out,g,5); break; }
        case 'm': { uint8_t g[5]={0x7C,0x04,0x18,0x04,0x78}; std::memcpy(out,g,5); break; }
        case 'p': { uint8_t g[5]={0x7C,0x14,0x14,0x14,0x08}; std::memcpy(out,g,5); break; }
        case 'r': { uint8_t g[5]={0x7C,0x08,0x04,0x04,0x08}; std::memcpy(out,g,5); break; }
        case 'a': { uint8_t g[5]={0x20,0x54,0x54,0x54,0x78}; std::memcpy(out,g,5); break; }
        case 't': { uint8_t g[5]={0x04,0x3F,0x44,0x40,0x20}; std::memcpy(out,g,5); break; }
        case 'u': { uint8_t g[5]={0x3C,0x40,0x40,0x20,0x7C}; std::memcpy(out,g,5); break; }
        default: break;
    }
}

} // namespace

bool SSD1306_Init(void)
{
    const uint8_t initSequence[] = {
        0xAE, 0x20, 0x02, 0xB0, 0xC8, 0x00, 0x10, 0x40,
        0x81, 0x7F, 0xA1, 0xA6, 0xA8, 0x3F, 0xA4, 0xD3,
        0x00, 0xD5, 0x80, 0xD9, 0xF1, 0xDA, 0x12, 0xDB,
        0x40, 0x8D, 0x14, 0xAF
    };

    for (uint8_t command : initSequence) {
        if (!SendCommand(command)) {
            return false;
        }
    }

    SSD1306_Clear();
    return SSD1306_Update();
}

void SSD1306_Clear(void)
{
    std::memset(framebuffer, 0, sizeof(framebuffer));
}

void SSD1306_DrawText(uint8_t x, uint8_t page, const char *text)
{
    if (page >= 8 || text == nullptr) {
        return;
    }

    while (*text != '\0' && x < 128) {
        uint8_t glyph[5];
        Glyph(*text++, glyph);

        for (uint8_t col = 0; col < 5 && x < 128; ++col) {
            framebuffer[static_cast<uint16_t>(page) * 128U + x++] = glyph[col];
        }
        if (x < 128) {
            framebuffer[static_cast<uint16_t>(page) * 128U + x++] = 0x00;
        }
    }
}

bool SSD1306_Update(void)
{
    uint8_t packet[129];
    packet[0] = 0x40;

    for (uint8_t page = 0U; page < 8U; ++page) {
        if (!SendCommand(static_cast<uint8_t>(0xB0U + page)) ||
            !SendCommand(0x00) || !SendCommand(0x10)) return false;

        std::memcpy(&packet[1], &framebuffer[static_cast<uint16_t>(page) * 128U], 128);
        if (HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDRESS, packet, sizeof(packet), 100) != HAL_OK) return false;
    }
    return true;
}

bool SSD1306_DisplayOff(void)
{
    return SendCommand(0xAE);
}

