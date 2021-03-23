/**
 * This Library is written and optimized by Olivier Van den Eede(4ilo) in 2016
 * for Stm32 Uc and HAL-i2c lib's.
 *
 * 変更点
 * ・C++へ変更し、std::stringを使用
 * ・文字を描画するスペースがなくても無理やり描画
 * ・xyWriteStrWT関数追加
 *
 */

#ifndef _SSD1306_HPP
#define _SSD1306_HPP

#include "stm32f7xx_hal.h"
#include <string>
#include "fonts.h"
using std::string;

// I2c address
#ifndef SSD1306_I2C_ADDR
#define SSD1306_I2C_ADDR        0x78
#endif // SSD1306_I2C_ADDR

// SSD1306 width in pixels
#ifndef SSD1306_WIDTH
#define SSD1306_WIDTH           128
#endif // SSD1306_WIDTH

// SSD1306 LCD height in pixels
#ifndef SSD1306_HEIGHT
#define SSD1306_HEIGHT          64
#endif // SSD1306_HEIGHT

//  Enumeration for screen colors
typedef enum {
    Black = 0x00,   // Black color, no pixel
    White = 0x01,   // Pixel is set. Color depends on LCD
} SSD1306_COLOR;

//  Struct to store transformations
typedef struct {
    uint16_t CurrentX;
    uint16_t CurrentY;
    uint8_t Inverted;
    uint8_t Initialized;
} SSD1306_t;

//  Function definitions

uint8_t ssd1306_Init(I2C_HandleTypeDef *hi2c);
void ssd1306_UpdateScreen(I2C_HandleTypeDef *hi2c);
void ssd1306_Fill(SSD1306_COLOR color);
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);
char ssd1306_WriteChar(char ch, FontDef Font, SSD1306_COLOR color);
char ssd1306_WriteString(string strn, FontDef Font, SSD1306_COLOR color);
void ssd1306_SetCursor(uint8_t x, uint8_t y);
void ssd1306_InvertColors(void);
void ssd1306_xyWriteStrWT(uint8_t x, uint8_t y, string str, FontDef Font);
void ssd1306_R_xyWriteStrWT(uint8_t x, uint8_t y, string str, FontDef Font);
void ssd1306_InvertPixel(uint8_t x, uint8_t y);

#endif  // _SSD1306_HPP
