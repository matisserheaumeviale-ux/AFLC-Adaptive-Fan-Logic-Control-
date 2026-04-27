#pragma once

#include "stm32f1xx_hal.h"
#include <stdint.h>

// API minimale pour l'ecran LCD 20x4.
void LCD_Init(void);
void LCD_Clear(void);
void LCD_SendCommand(uint8_t cmd);
void LCD_SendChar(char data);
void LCD_SendString(const char *str);
void LCD_SetCursor(uint8_t col, uint8_t row);

void LCD_WriteAt(uint8_t col, uint8_t row, const char *str);
void LCD_Printf(const char *fmt, ...);
void LCD_PrintfAt(uint8_t col, uint8_t row, const char *fmt, ...);

// Helpers de confort.
void LCD_ClearLine(uint8_t row);
void LCD_ScrollText(const char *text);
