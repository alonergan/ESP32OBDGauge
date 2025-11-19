#ifndef CONFIG_H
#define CONFIG_H

/*
  Versioning and other descriptors
*/
#define SOFTWARE_VERSION "0.1"
#define DEVICE_DESCRIPTION "ESP32 DevKitC-V1"

/*
  Hardware config
*/
#define BUTTON_PIN 14


/*
  Bluetooth config
*/
#define BLUETOOTH_DEVICE_ADDRESS "8c:de:52:dc:5d:2a"         // OBDII BLE device MAC address
#define BLUETOOTH_SERVICE_UUID "FFF0"                        // OBDII BLE main service UUID
#define BLUETOOTH_WRITE_CHAR_UUID "FFF2"                     // OBDII BLE write/read characteristic UUID
#define BLUETOOTH_NOTIFY_CHAR_UUID "FFF1"                    // OBDII BLE write/notify characteristic UUID

/*
  Display config
*/
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define DISPLAY_CENTER_X DISPLAY_WIDTH / 2
#define DISPLAY_CENTER_Y DISPLAY_HEIGHT / 2
#define DISPLAY_BG_COLOR TFT_BLACK
#define FONT_NORMAL_8 &AudiType_Normal_038pt7b
#define FONT_NORMAL_12 &AudiType_Normal_0312pt7b
#define FONT_NORMAL_14 &AudiType_Normal_0314pt7b
#define FONT_NORMAL_16 &AudiType_Normal_0316pt7b
#define FONT_NORMAL_18 &AudiType_Normal_0318pt7b
#define FONT_BOLD_8 &AudiType_Bold_038pt7b
#define FONT_BOLD_12 &AudiType_Bold_0312pt7b
#define FONT_BOLD_14 &AudiType_Bold_0314pt7b
#define FONT_BOLD_16 &AudiType_Bold_0316pt7b
#define FONT_BOLD_18 &AudiType_Bold_0318pt7b


/*
  Gauge params
*/
#define GAUGE_WIDTH DISPLAY_HEIGHT + 2 // Add a small increase to width to prevent chord in arc outline
#define GAUGE_HEIGHT DISPLAY_HEIGHT
#define GAUGE_RADIUS GAUGE_HEIGHT / 2
#define GAUGE_MARGIN_TOP 20
#define GAUGE_ARC_WIDTH 50
#define GAUGE_LINE_WIDTH 3
#define GAUGE_START_ANGLE 60
#define GAUGE_END_ANGLE 300
#define GAUGE_FG_COLOR TFT_WHITE
#define GAUGE_BG_COLOR DISPLAY_BG_COLOR
#define GAUGE_LABEL_SIZE 3
#define GAUGE_UNITS_SIZE 2
#define SMOOTHING_FACTOR 0.20 // Alpha smoothing factor for interpolation of values

/*
  Tick Mark Params
*/
#define MAJOR_TICK_SPACING 30 // Angular spacing for major ticks in degrees
#define MINOR_TICK_SPACING 10 // Angular spacing for minor ticks in degrees
#define MAJOR_TICK_LENGTH 12  // Length of major ticks in pixels
#define MINOR_TICK_LENGTH 6   // Length of minor ticks in pixels
#define MAJOR_TICK_THICKNESS 3 // Thickness of major ticks in pixels
#define MINOR_TICK_THICKNESS 1 // Thickness of minor ticks in pixels

/* 
  Needle Params 
*/
#define NEEDLE_SPRITE_WIDTH 5
#define NEEDLE_WIDTH 5
#define NEEDLE_LENGTH 45
#define NEEDLE_RADIUS 115
#define NEEDLE_COLOR_PRIMARY TFT_RED
#define NEEDLE_UPDATE_THRESHOLD 1

/* 
  Value Params 
*/
#define VALUE_WIDTH 120
#define VALUE_HEIGHT 45
#define VALUE_X (DISPLAY_WIDTH - VALUE_WIDTH) / 2
#define VALUE_Y 180
#define VALUE_BG_COLOR DISPLAY_BG_COLOR
#define VALUE_TEXT_COLOR TFT_WHITE
#define VALUE_FONT_SIZE 1

/*
  Dual Gauge Params
*/
#define DG_BAR_WIDTH 60
#define DG_BAR_HEIGHT 150
#define DG_BAR_LINE_WIDTH 2
#define DG_LABEL_SIZE 2
#define DG_UNITS_SIZE 1
#define DG_VALUE_SIZE 3

/*
  GMeter Params
*/
#define GMETER_RADIUS 90  
#define GMETER_WIDTH (2 * GMETER_RADIUS)
#define GMETER_HEIGHT (2 * GMETER_RADIUS)
#define GMETER_PADDING 10
#define GMETER_TEXT_SIZE 2
#define GMETER_TEXT_FONT 1
#define GMETER_TEXT_COLOR TFT_RED
#define GMETER_TEXT_OFFSET_X 35
#define GMETER_TEXT_OFFSET_Y 15  
#define GMETER_POINT_RADIUS 5
#define GMETER_POINT_COLOR TFT_RED
#define GMETER_OUTLINE_COLOR TFT_WHITE
#define GMETER_LINE_SIZE 3
#define GMETER_HISTORY_COLOR 0x9cf3

/*
  Acceleration Meter Params
*/
#define AMETER_SPEED_LABEL "Speed"
#define AMETER_TIME_LABEL "Time"
#define AMETER_TIME_COLOR TFT_RED
#define AMETER_SPEED_COLOR TFT_RED
#define AMETER_V_PADDING 10


#endif // CONFIG_H