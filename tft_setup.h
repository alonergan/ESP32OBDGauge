#ifndef ESP32_OBD_TFT_SETUP_H
#define ESP32_OBD_TFT_SETUP_H

// Project-local TFT_eSPI configuration. TFT_eSPI 2.5.x automatically detects
// this file, so users do not need to edit User_Setup.h or User_Setup_Select.h.
#define USER_SETUP_INFO "ESP32OBDGauge local setup"

// 320x240 ILI9341 display (portrait dimensions; the sketch rotates it).
#define ILI9341_2_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 320
#define TFT_INVERSION_ON

// ESP32 DevKitC-V1 SPI wiring.
#define TFT_MISO -1
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS    2
#define TFT_DC    5
#define TFT_RST   4

// Only enable the built-in fonts used by the UI plus GFX custom-font support.
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY       70000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY 2500000

#endif
