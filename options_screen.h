#ifndef OPTIONS_SCREEN_H
#define OPTIONS_SCREEN_H

#include <TFT_eSPI.h>
#include "config.h"
#include "ui_library.h"
#include "touch.h"
#include "gauge.h"
#include "needle_gauge.h"
#include "dual_gauge.h"
#include "g_meter.h"

class OptionsScreen {
public:
    OptionsScreen(TFT_eSPI* display, Gauge** gauges, int numGauges)
        : display(display), gauges(gauges), numGauges(numGauges), screenSprite(display), state(MAIN_MENU), colorState(MAIN_COLOR_MENU) {}

    void initialize() {
        display->fillScreen(TFT_BLACK);
        if (!screenSprite.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT)) {
            Serial.println("Failed to create options screen sprite");
            return;
        }
        drawMainMenu();
    }

    bool handleTouch(uint16_t x, uint16_t y) {
        return handleTouchGesture({TouchGesture::TAP, static_cast<int16_t>(x), static_cast<int16_t>(y), static_cast<int16_t>(x), static_cast<int16_t>(y), 0, 0, 1.0f});
    }

    bool handleTouchGesture(const TouchGesture& gesture) {
        if (gesture.type == TouchGesture::SWIPE_RIGHT || gesture.type == TouchGesture::SWIPE_DOWN) {
            if (state == MAIN_MENU) {
                return false;
            }
            goBack();
            return true;
        }

        int16_t x = gesture.endX;
        int16_t y = gesture.endY;

        if (state == MAIN_MENU) {
            return handleMainMenuTouch(x, y);
        }

        if (state == ABOUT) {
            return handleAboutTouch(x, y);
        }

        if (state == BLUETOOTH) {
            return handleBluetoothTouch(x, y);
        }

        if (state == COLOR) {
            return handleColorTouch(x, y);
        }

        return true;
    }

private:
    TFT_eSPI* display;
    Gauge** gauges;
    int numGauges;
    TFT_eSprite screenSprite;

    enum ScreenState { MAIN_MENU, ABOUT, BLUETOOTH, COLOR };
    enum ColorState { MAIN_COLOR_MENU, NEEDLE_COLOR, OUTLINE_COLOR, VALUE_COLOR };
    enum BluetoothState { BLUETOOTH_MENU, BLUETOOTH_STATS };
    ScreenState state;
    ColorState colorState;
    BluetoothState bluetoothState = BLUETOOTH_MENU;

    static const int BUTTON_WIDTH = 140;
    static const int BUTTON_HEIGHT = 100;
    static const int BUTTON_SPACING = 20;
    static const int BUTTON_MARGIN = 10;
    static const int COLOR_SWATCH_SIZE = 60;

    uint16_t colorOptions[12] = {
        TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW,
        TFT_ORANGE, TFT_DARKGREEN, TFT_CYAN, TFT_GOLD,
        TFT_VIOLET, TFT_PURPLE, TFT_SILVER, TFT_WHITE
    };

    UIButton buildGridButton(int id, int row, int col, const char* label, uint16_t color = TFT_DARKGREY) {
        UIRect rect = {
            static_cast<int16_t>(BUTTON_MARGIN + col * (BUTTON_WIDTH + BUTTON_SPACING)),
            static_cast<int16_t>(BUTTON_MARGIN + row * (BUTTON_HEIGHT + BUTTON_SPACING)),
            static_cast<int16_t>(BUTTON_WIDTH),
            static_cast<int16_t>(BUTTON_HEIGHT)
        };
        return UIButton(id, label, rect, color, TFT_WHITE, TFT_WHITE);
    }

    void drawButtons(UIButton* buttons, int count) {
        for (int i = 0; i < count; i++) {
            buttons[i].draw(screenSprite, 2, 1);
        }
    }

    int hitButton(UIButton* buttons, int count, int16_t x, int16_t y) {
        for (int i = 0; i < count; i++) {
            if (buttons[i].hitTest(x, y)) {
                return buttons[i].getId();
            }
        }
        return -1;
    }

    bool handleMainMenuTouch(uint16_t x, uint16_t y) {
        UIButton buttons[4] = {
            buildGridButton(0, 0, 0, "G-Meter"),
            buildGridButton(1, 0, 1, "Bluetooth"),
            buildGridButton(2, 1, 0, "Color"),
            buildGridButton(3, 1, 1, "Exit")
        };

        int buttonId = hitButton(buttons, 4, x, y);
        switch (buttonId) {
            case 0:
                state = ABOUT;
                drawAboutMenu();
                return true;
            case 1:
                state = BLUETOOTH;
                bluetoothState = BLUETOOTH_MENU;
                drawBluetoothMenu();
                return true;
            case 2:
                state = COLOR;
                colorState = MAIN_COLOR_MENU;
                drawColorMenu();
                return true;
            case 3:
                return false;
            default:
                return true;
        }
    }

    bool handleAboutTouch(uint16_t x, uint16_t y) {
        UIButton buttons[2] = {
            UIButton(0, "Start Calibration", {65, 90, 190, 55}, TFT_DARKGREEN),
            UIButton(1, "Back", {65, 160, 190, 55})
        };

        int buttonId = hitButton(buttons, 2, x, y);
        if (buttonId == 0) {
            triggerGMeterCalibration();
            drawAboutMenu();
            return true;
        }
        if (buttonId == 1) {
            state = MAIN_MENU;
            drawMainMenu();
            return true;
        }
        return true;
    }

    bool handleColorTouch(uint16_t x, uint16_t y) {
        if (colorState == MAIN_COLOR_MENU) {
            UIButton buttons[4] = {
                buildGridButton(0, 0, 0, "Needle Color"),
                buildGridButton(1, 0, 1, "Outline Color"),
                buildGridButton(2, 1, 0, "Value Color"),
                buildGridButton(3, 1, 1, "Exit")
            };

            int buttonId = hitButton(buttons, 4, x, y);
            if (buttonId == 0) {
                colorState = NEEDLE_COLOR;
                drawColorPicker();
            } else if (buttonId == 1) {
                colorState = OUTLINE_COLOR;
                drawColorPicker();
            } else if (buttonId == 2) {
                colorState = VALUE_COLOR;
                drawColorPicker();
            } else if (buttonId == 3) {
                state = MAIN_MENU;
                colorState = MAIN_COLOR_MENU;
                drawMainMenu();
            }
            return true;
        }

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 4; j++) {
                UIRect swatch = {
                    static_cast<int16_t>(BUTTON_MARGIN + j * (COLOR_SWATCH_SIZE + BUTTON_SPACING)),
                    static_cast<int16_t>(BUTTON_MARGIN + i * (COLOR_SWATCH_SIZE + BUTTON_SPACING)),
                    COLOR_SWATCH_SIZE,
                    COLOR_SWATCH_SIZE
                };

                if (swatch.contains(x, y)) {
                    uint16_t color = colorOptions[i * 4 + j];
                    if (colorState == NEEDLE_COLOR) {
                        updateNeedleColor(color);
                    } else if (colorState == OUTLINE_COLOR) {
                        updateOutlineColor(color);
                    } else if (colorState == VALUE_COLOR) {
                        updateValueColor(color);
                    }
                    colorState = MAIN_COLOR_MENU;
                    drawColorMenu();
                    return true;
                }
            }
        }

        return true;
    }

    bool handleBluetoothTouch(uint16_t x, uint16_t y) {
        if (bluetoothState == BLUETOOTH_STATS) {
            UIButton backButton(0, "Back", {10, 205, 300, 30});
            if (backButton.hitTest(x, y)) {
                bluetoothState = BLUETOOTH_MENU;
                drawBluetoothMenu();
            }
            return true;
        }

        UIButton buttons[4] = {
            buildGridButton(0, 0, 0, "Pair Device"),
            buildGridButton(1, 0, 1, "Stats"),
            buildGridButton(2, 1, 0, "Remove Device"),
            buildGridButton(3, 1, 1, "Exit")
        };

        int buttonId = hitButton(buttons, 4, x, y);
        if (buttonId == 1) {
            bluetoothState = BLUETOOTH_STATS;
            drawBluetoothStats();
            return true;
        }
        if (buttonId == 3) {
            state = MAIN_MENU;
            bluetoothState = BLUETOOTH_MENU;
            drawMainMenu();
            return true;
        }
        return true;
    }

    void drawMainMenu() {
        screenSprite.fillSprite(TFT_BLACK);
        UIButton buttons[4] = {
            buildGridButton(0, 0, 0, "G-Meter"),
            buildGridButton(1, 0, 1, "Bluetooth"),
            buildGridButton(2, 1, 0, "Color"),
            buildGridButton(3, 1, 1, "Exit")
        };
        drawButtons(buttons, 4);
        screenSprite.pushSprite(0, 0);
    }

    void drawAboutMenu() {
        screenSprite.fillSprite(TFT_BLACK);
        screenSprite.setTextFont(2);
        screenSprite.setTextSize(2);
        screenSprite.setTextColor(TFT_WHITE);

        String title = "G-Meter Calibration";
        int titleWidth = screenSprite.textWidth(title);
        screenSprite.setCursor(DISPLAY_CENTER_X - (titleWidth / 2), 24);
        screenSprite.print(title);

        screenSprite.setTextSize(1);
        String text = "Park on flat ground before calibrating.";
        int textWidth = screenSprite.textWidth(text);
        screenSprite.setCursor(DISPLAY_CENTER_X - (textWidth / 2), 55);
        screenSprite.print(text);

        UIButton buttons[2] = {
            UIButton(0, "Start Calibration", {65, 90, 190, 55}, TFT_DARKGREEN),
            UIButton(1, "Back", {65, 160, 190, 55})
        };
        drawButtons(buttons, 2);
        screenSprite.pushSprite(0, 0);
    }

    void drawBluetoothMenu() {
        bluetoothState = BLUETOOTH_MENU;
        screenSprite.fillSprite(TFT_BLACK);
        UIButton buttons[4] = {
            buildGridButton(0, 0, 0, "Pair Device"),
            buildGridButton(1, 0, 1, "Stats"),
            buildGridButton(2, 1, 0, "Remove Device"),
            buildGridButton(3, 1, 1, "Exit")
        };
        drawButtons(buttons, 4);
        screenSprite.pushSprite(0, 0);
    }

    void drawBluetoothStats() {
        screenSprite.fillSprite(TFT_BLACK);

        UITable statsTable;
        statsTable.configure({5, 5, 310, 190}, 4, 2);
        statsTable.setCell(0, 0, "Metric");
        statsTable.setCell(0, 1, "Value");
        statsTable.setCell(1, 0, "Connected");
        statsTable.setCell(1, 1, "Yes");
        statsTable.setCell(2, 0, "Device");
        statsTable.setCell(2, 1, "OBD BLE");
        statsTable.setCell(3, 0, "Version");
        statsTable.setCell(3, 1, SOFTWARE_VERSION);
        statsTable.draw(screenSprite, true);

        UIButton backButton(0, "Back", {10, 205, 300, 30});
        backButton.draw(screenSprite, 2, 1);
        screenSprite.pushSprite(0, 0);
    }

    void drawColorMenu() {
        screenSprite.fillSprite(TFT_BLACK);
        UIButton buttons[4] = {
            buildGridButton(0, 0, 0, "Needle Color"),
            buildGridButton(1, 0, 1, "Outline Color"),
            buildGridButton(2, 1, 0, "Value Color"),
            buildGridButton(3, 1, 1, "Exit")
        };
        drawButtons(buttons, 4);
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
        screenSprite.pushSprite(0, 0);
    }

    void goBack() {
        if (state == COLOR && colorState != MAIN_COLOR_MENU) {
            colorState = MAIN_COLOR_MENU;
            drawColorMenu();
            return;
        }

        if (state == BLUETOOTH && bluetoothState == BLUETOOTH_STATS) {
            bluetoothState = BLUETOOTH_MENU;
            drawBluetoothMenu();
            return;
        }

        state = MAIN_MENU;
        colorState = MAIN_COLOR_MENU;
        bluetoothState = BLUETOOTH_MENU;
        drawMainMenu();
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
            if (gauges[i]->getType() == Gauge::NEEDLE_GAUGE) {
                static_cast<NeedleGauge*>(gauges[i])->setNeedleColor(color);
            }
            if (gauges[i]->getType() == Gauge::DUAL_GAUGE) {
                static_cast<DualGauge*>(gauges[i])->setNeedleColor(color);
            }
        }
    }

    void updateOutlineColor(uint16_t color) {
        for (int i = 0; i < numGauges; i++) {
            if (gauges[i]->getType() == Gauge::NEEDLE_GAUGE) {
                static_cast<NeedleGauge*>(gauges[i])->setOutlineColor(color);
            }
            if (gauges[i]->getType() == Gauge::DUAL_GAUGE) {
                static_cast<DualGauge*>(gauges[i])->setOutlineColor(color);
            }
        }
    }

    void updateValueColor(uint16_t color) {
        for (int i = 0; i < numGauges; i++) {
            if (gauges[i]->getType() == Gauge::NEEDLE_GAUGE) {
                static_cast<NeedleGauge*>(gauges[i])->setValueColor(color);
            }
            if (gauges[i]->getType() == Gauge::DUAL_GAUGE) {
                static_cast<DualGauge*>(gauges[i])->setValueColor(color);
            }
        }
    }
};

#endif
