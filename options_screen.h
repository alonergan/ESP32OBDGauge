#ifndef OPTIONS_SCREEN_H
#define OPTIONS_SCREEN_H

#include <TFT_eSPI.h>
#include "config.h"
#include "gauge.h"
#include "g_meter.h"
#include "needle_gauge.h"
#include "dual_gauge.h"
#include "ui_components.h"
#include "ui_touch.h"

class OptionsScreen {
public:
  OptionsScreen(TFT_eSPI* display, Gauge** gauges, int numGauges)
      : display(display), gauges(gauges), numGauges(numGauges), screenSprite(display), state(MAIN_MENU), colorState(MAIN_COLOR_MENU),
        bluetoothState(MAIN_BLUETOOTH_MENU) {}

  void initialize() {
    display->fillScreen(TFT_BLACK);
    if (!screenSprite.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT)) {
      Serial.println("Failed to create options screen sprite");
      return;
    }
    drawMainMenu();
  }

  bool handleTouch(uint16_t x, uint16_t y) {
    switch (state) {
      case MAIN_MENU:
        return handleMainMenuTap(x, y);
      case ABOUT:
        return handleAboutTap(x, y);
      case BLUETOOTH:
        return handleBluetoothTap(x, y);
      case COLOR:
        return handleColorTap(x, y);
    }
    return true;
  }

  bool handleGesture(TouchGestureEngine::GestureType gesture) {
    if (gesture == TouchGestureEngine::SWIPE_RIGHT || gesture == TouchGestureEngine::SWIPE_DOWN) {
      if (state == MAIN_MENU) {
        return false;
      }
      if (state == COLOR && colorState != MAIN_COLOR_MENU) {
        colorState = MAIN_COLOR_MENU;
        drawColorMenu();
        return true;
      }
      if (state == BLUETOOTH && bluetoothState != MAIN_BLUETOOTH_MENU) {
        bluetoothState = MAIN_BLUETOOTH_MENU;
        drawBluetoothMenu();
        return true;
      }
      state = MAIN_MENU;
      drawMainMenu();
      return true;
    }
    return true;
  }

private:
  enum ScreenState { MAIN_MENU, ABOUT, BLUETOOTH, COLOR };
  enum ColorState { MAIN_COLOR_MENU, NEEDLE_COLOR, OUTLINE_COLOR, VALUE_COLOR };
  enum BluetoothState { MAIN_BLUETOOTH_MENU, STATS_MENU };

  TFT_eSPI* display;
  Gauge** gauges;
  int numGauges;
  TFT_eSprite screenSprite;
  ScreenState state;
  ColorState colorState;
  BluetoothState bluetoothState;

  static const int BUTTON_MARGIN = 10;
  static const int BUTTON_SPACING = 20;
  static const int COLOR_SWATCH_SIZE = 60;

  uint16_t colorOptions[12] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW, TFT_ORANGE, TFT_DARKGREEN,
                               TFT_CYAN, TFT_GOLD, TFT_VIOLET, TFT_PURPLE, TFT_SILVER, TFT_WHITE};

  bool handleMainMenuTap(uint16_t x, uint16_t y) {
    UIButtonGrid grid;
    grid.configure({BUTTON_MARGIN, BUTTON_MARGIN, DISPLAY_WIDTH - (2 * BUTTON_MARGIN), DISPLAY_HEIGHT - (2 * BUTTON_MARGIN)}, 2, 2, BUTTON_SPACING);
    int8_t index = grid.hitTest(x, y);

    if (index == 0) {
      state = ABOUT;
      drawAboutMenu();
      return true;
    }
    if (index == 1) {
      state = BLUETOOTH;
      bluetoothState = MAIN_BLUETOOTH_MENU;
      drawBluetoothMenu();
      return true;
    }
    if (index == 2) {
      state = COLOR;
      colorState = MAIN_COLOR_MENU;
      drawColorMenu();
      return true;
    }
    if (index == 3) {
      return false;
    }
    return true;
  }

  bool handleAboutTap(uint16_t x, uint16_t y) {
    UIButton calibrate({65, 90, 190, 55}, "Start Calibration", TFT_DARKGREEN);
    UIButton back({65, 160, 190, 55}, "Back", TFT_DARKGREY);

    if (calibrate.hitTest(x, y)) {
      triggerGMeterCalibration();
      drawAboutMenu();
    } else if (back.hitTest(x, y)) {
      state = MAIN_MENU;
      drawMainMenu();
    }
    return true;
  }

  bool handleBluetoothTap(uint16_t x, uint16_t y) {
    if (bluetoothState == STATS_MENU) {
      UIButton back({65, 185, 190, 45}, "Back", TFT_DARKGREY);
      if (back.hitTest(x, y)) {
        bluetoothState = MAIN_BLUETOOTH_MENU;
        drawBluetoothMenu();
      }
      return true;
    }

    UIButtonGrid grid;
    grid.configure({BUTTON_MARGIN, BUTTON_MARGIN, DISPLAY_WIDTH - (2 * BUTTON_MARGIN), DISPLAY_HEIGHT - (2 * BUTTON_MARGIN)}, 2, 2, BUTTON_SPACING);
    int8_t index = grid.hitTest(x, y);

    if (index == 0) {
      drawBluetoothMessage("Pairing managed by\nOBD connection flow");
    } else if (index == 1) {
      bluetoothState = STATS_MENU;
      drawBluetoothStats();
    } else if (index == 2) {
      drawBluetoothMessage("Remove not implemented");
    } else if (index == 3) {
      state = MAIN_MENU;
      drawMainMenu();
    }
    return true;
  }

  bool handleColorTap(uint16_t x, uint16_t y) {
    if (colorState == MAIN_COLOR_MENU) {
      UIButtonGrid grid;
      grid.configure({BUTTON_MARGIN, BUTTON_MARGIN, DISPLAY_WIDTH - (2 * BUTTON_MARGIN), DISPLAY_HEIGHT - (2 * BUTTON_MARGIN)}, 2, 2, BUTTON_SPACING);
      int8_t index = grid.hitTest(x, y);

      if (index == 0) {
        colorState = NEEDLE_COLOR;
        drawColorPicker();
      } else if (index == 1) {
        colorState = OUTLINE_COLOR;
        drawColorPicker();
      } else if (index == 2) {
        colorState = VALUE_COLOR;
        drawColorPicker();
      } else if (index == 3) {
        state = MAIN_MENU;
        drawMainMenu();
      }
      return true;
    }

    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 4; j++) {
        UIRect swatch = {BUTTON_MARGIN + j * (COLOR_SWATCH_SIZE + BUTTON_SPACING), BUTTON_MARGIN + i * (COLOR_SWATCH_SIZE + BUTTON_SPACING), COLOR_SWATCH_SIZE,
                         COLOR_SWATCH_SIZE};
        if (swatch.contains(x, y)) {
          uint16_t selectedColor = colorOptions[i * 4 + j];
          if (colorState == NEEDLE_COLOR)
            updateNeedleColor(selectedColor);
          else if (colorState == OUTLINE_COLOR)
            updateOutlineColor(selectedColor);
          else
            updateValueColor(selectedColor);

          colorState = MAIN_COLOR_MENU;
          drawColorMenu();
          return true;
        }
      }
    }

    return true;
  }

  void drawMainMenu() {
    screenSprite.fillSprite(TFT_BLACK);
    UIButtonGrid grid;
    grid.configure({BUTTON_MARGIN, BUTTON_MARGIN, DISPLAY_WIDTH - (2 * BUTTON_MARGIN), DISPLAY_HEIGHT - (2 * BUTTON_MARGIN)}, 2, 2, BUTTON_SPACING);
    grid.setButton(0, "G-Meter", TFT_DARKGREEN);
    grid.setButton(1, "Bluetooth");
    grid.setButton(2, "Colors");
    grid.setButton(3, "Exit");
    grid.draw(screenSprite);
    screenSprite.pushSprite(0, 0);
  }

  void drawAboutMenu() {
    screenSprite.fillSprite(TFT_BLACK);
    screenSprite.setTextFont(2);
    screenSprite.setTextSize(2);
    screenSprite.setTextColor(TFT_WHITE);
    screenSprite.setCursor(60, 24);
    screenSprite.print("G-Meter Calibration");
    screenSprite.setTextSize(1);
    screenSprite.setCursor(35, 55);
    screenSprite.print("Park on flat ground before calibrating.");

    UIButton({65, 90, 190, 55}, "Start Calibration", TFT_DARKGREEN).draw(screenSprite);
    UIButton({65, 160, 190, 55}, "Back", TFT_DARKGREY).draw(screenSprite);
    screenSprite.pushSprite(0, 0);
  }

  void drawBluetoothMenu() {
    screenSprite.fillSprite(TFT_BLACK);
    UIButtonGrid grid;
    grid.configure({BUTTON_MARGIN, BUTTON_MARGIN, DISPLAY_WIDTH - (2 * BUTTON_MARGIN), DISPLAY_HEIGHT - (2 * BUTTON_MARGIN)}, 2, 2, BUTTON_SPACING);
    grid.setButton(0, "Pair Device");
    grid.setButton(1, "Stats", TFT_DARKGREEN);
    grid.setButton(2, "Remove", TFT_MAROON);
    grid.setButton(3, "Exit");
    grid.draw(screenSprite);
    screenSprite.pushSprite(0, 0);
  }

  void drawBluetoothStats() {
    screenSprite.fillSprite(TFT_BLACK);
    UITable table;
    table.setBounds({20, 20, DISPLAY_WIDTH - 40, 150});
    table.setHeader("Bluetooth Status", TFT_NAVY);
    table.addRow("Screen", "Options");
    table.addRow("Connected", "Managed globally");
    table.addRow("Action", "Swipe right to back");
    table.addRow("Version", SOFTWARE_VERSION);
    table.draw(screenSprite);
    UIButton({65, 185, 190, 45}, "Back", TFT_DARKGREY).draw(screenSprite);
    screenSprite.pushSprite(0, 0);
  }

  void drawBluetoothMessage(const char* message) {
    screenSprite.fillSprite(TFT_BLACK);
    screenSprite.setTextFont(2);
    screenSprite.setTextSize(1);
    screenSprite.setTextColor(TFT_WHITE);
    screenSprite.setCursor(25, 70);
    screenSprite.print(message);
    screenSprite.setCursor(25, 100);
    screenSprite.print("Tap anywhere to continue");
    screenSprite.pushSprite(0, 0);
    delay(500);
    drawBluetoothMenu();
  }

  void drawColorMenu() {
    screenSprite.fillSprite(TFT_BLACK);
    UIButtonGrid grid;
    grid.configure({BUTTON_MARGIN, BUTTON_MARGIN, DISPLAY_WIDTH - (2 * BUTTON_MARGIN), DISPLAY_HEIGHT - (2 * BUTTON_MARGIN)}, 2, 2, BUTTON_SPACING);
    grid.setButton(0, "Needle", TFT_DARKGREEN);
    grid.setButton(1, "Outline", TFT_NAVY);
    grid.setButton(2, "Value", TFT_PURPLE);
    grid.setButton(3, "Exit");
    grid.draw(screenSprite);
    screenSprite.pushSprite(0, 0);
  }

  void drawColorPicker() {
    screenSprite.fillSprite(TFT_BLACK);
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 4; j++) {
        int x = BUTTON_MARGIN + j * (COLOR_SWATCH_SIZE + BUTTON_SPACING);
        int y = BUTTON_MARGIN + i * (COLOR_SWATCH_SIZE + BUTTON_SPACING);
        screenSprite.fillRect(x, y, COLOR_SWATCH_SIZE, COLOR_SWATCH_SIZE, colorOptions[i * 4 + j]);
        screenSprite.drawRect(x, y, COLOR_SWATCH_SIZE, COLOR_SWATCH_SIZE, TFT_WHITE);
      }
    }
    screenSprite.setTextColor(TFT_WHITE);
    screenSprite.setTextSize(1);
    screenSprite.setTextFont(2);
    screenSprite.setCursor(15, 220);
    screenSprite.print("Select a color or swipe right to return");
    screenSprite.pushSprite(0, 0);
  }

  void triggerGMeterCalibration() {
    for (int i = 0; i < numGauges; i++) {
      if (gauges[i]->getType() == Gauge::G_METER) {
        static_cast<GMeter*>(gauges[i])->beginManualCalibration();
        break;
      }
    }
  }

  void updateNeedleColor(uint16_t color) {
    for (int i = 0; i < numGauges; i++) {
      if (gauges[i]->getType() == Gauge::NEEDLE_GAUGE) static_cast<NeedleGauge*>(gauges[i])->setNeedleColor(color);
      if (gauges[i]->getType() == Gauge::DUAL_GAUGE) static_cast<DualGauge*>(gauges[i])->setNeedleColor(color);
    }
  }

  void updateOutlineColor(uint16_t color) {
    for (int i = 0; i < numGauges; i++) {
      if (gauges[i]->getType() == Gauge::NEEDLE_GAUGE) static_cast<NeedleGauge*>(gauges[i])->setOutlineColor(color);
      if (gauges[i]->getType() == Gauge::DUAL_GAUGE) static_cast<DualGauge*>(gauges[i])->setOutlineColor(color);
    }
  }

  void updateValueColor(uint16_t color) {
    for (int i = 0; i < numGauges; i++) {
      if (gauges[i]->getType() == Gauge::NEEDLE_GAUGE) static_cast<NeedleGauge*>(gauges[i])->setValueColor(color);
      if (gauges[i]->getType() == Gauge::DUAL_GAUGE) static_cast<DualGauge*>(gauges[i])->setValueColor(color);
    }
  }
};

#endif
