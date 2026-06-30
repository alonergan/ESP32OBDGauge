#ifndef OPTIONS_SCREEN_H
#define OPTIONS_SCREEN_H

#include <TFT_eSPI.h>
#include <math.h>
#include "config.h"
#include "ui_library.h"
#include "touch.h"
#include "gauge.h"
#include "commands.h"
#include "needle_gauge.h"
#include "dual_gauge.h"
#include "g_meter.h"

class OptionsScreen {
public:
    OptionsScreen(TFT_eSPI* display, Gauge** gauges, int numGauges, Commands* commands)
        : display(display), gauges(gauges), numGauges(numGauges), commands(commands),
          screenSprite(display), state(MAIN_MENU), listStart(0), listTarget(SINGLE_TARGET),
          colorTarget(VALUE_TARGET), pickerHue(0.0f), pickerSaturation(1.0f),
          pickerValue(1.0f), pendingColor(TFT_RED) {}

    void initialize() {
        display->fillScreen(TFT_BLACK);
        if (!screenSprite.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT)) {
            Serial.println("Failed to create options screen sprite");
            return;
        }
        drawCurrentScreen();
    }

    bool handleTouchGesture(const TouchGesture& gesture) {
        if (gesture.type == TouchGesture::SWIPE_RIGHT || gesture.type == TouchGesture::SWIPE_DOWN) {
            return navigateBack();
        }
        if (gesture.type != TouchGesture::TAP) return true;

        const int16_t x = gesture.endX;
        const int16_t y = gesture.endY;
        switch (state) {
            case MAIN_MENU: return handleMainMenu(x, y);
            case SETTINGS_MENU: return handleSettingsMenu(x, y);
            case BLUETOOTH_MENU: return handleBluetoothMenu(x, y);
            case BLUETOOTH_STATS: state = BLUETOOTH_MENU; drawCurrentScreen(); return true;
            case GMETER_MENU: return handleGMeterMenu(x, y);
            case DEVICE_MENU: state = SETTINGS_MENU; drawCurrentScreen(); return true;
            case GAUGE_TYPE_MENU: return handleGaugeTypeMenu(x, y);
            case SINGLE_LIST:
            case DUAL_LIST: return handleCommandList(x, y);
            case DUAL_MENU: return handleDualMenu(x, y);
            case COLOR_MENU: return handleColorMenu(x, y);
            case COLOR_PICKER: return handleColorPicker(x, y);
        }
        return true;
    }

private:
    enum ScreenState {
        MAIN_MENU, SETTINGS_MENU, BLUETOOTH_MENU, BLUETOOTH_STATS, GMETER_MENU,
        DEVICE_MENU, GAUGE_TYPE_MENU, SINGLE_LIST, DUAL_MENU, DUAL_LIST,
        COLOR_MENU, COLOR_PICKER
    };
    enum ListTarget { SINGLE_TARGET, DUAL_LEFT_TARGET, DUAL_RIGHT_TARGET };
    enum ColorTarget { VALUE_TARGET, LABEL_TARGET, OUTLINE_TARGET };

    TFT_eSPI* display;
    Gauge** gauges;
    int numGauges;
    Commands* commands;
    TFT_eSprite screenSprite;
    ScreenState state;
    int listStart;
    ListTarget listTarget;
    ColorTarget colorTarget;
    float pickerHue;
    float pickerSaturation;
    float pickerValue;
    uint16_t pendingColor;

    static const int GRID_MARGIN = 10;
    static const int GRID_SPACING = 10;
    static const int GRID_WIDTH = 145;
    static const int GRID_HEIGHT = 105;
    static const int LIST_TOP = 24;
    static const int LIST_ROW_HEIGHT = 32;
    static const int LIST_ROWS = 5;
    static const int LIST_BUTTON_Y = 198;

    void drawCurrentScreen() {
        switch (state) {
            case MAIN_MENU: {
                const char* labels[] = {"Settings", "Gauge Type", "Color", "Exit"};
                drawGrid(labels, 4); break;
            }
            case SETTINGS_MENU: {
                const char* labels[] = {"Bluetooth", "G-Meter", "Device", "Exit"};
                drawGrid(labels, 4); break;
            }
            case BLUETOOTH_MENU: drawBluetoothMenu(); break;
            case BLUETOOTH_STATS: drawBluetoothStats(); break;
            case GMETER_MENU: drawGMeterMenu(); break;
            case DEVICE_MENU: drawDeviceMenu(); break;
            case GAUGE_TYPE_MENU: {
                const char* labels[] = {"Single Gauge", "Dual Gauge", "Exit"};
                drawGrid(labels, 3); break;
            }
            case SINGLE_LIST:
            case DUAL_LIST: drawCommandList(); break;
            case DUAL_MENU: drawDualMenu(); break;
            case COLOR_MENU: {
                const char* labels[] = {"Value Color", "Label Color", "Outline Color", "Exit"};
                drawGrid(labels, 4); break;
            }
            case COLOR_PICKER: drawColorPicker(); break;
        }
    }

    UIRect gridBounds(int index) const {
        int row = index / 2;
        int col = index % 2;
        return {static_cast<int16_t>(GRID_MARGIN + col * (GRID_WIDTH + GRID_SPACING)),
                static_cast<int16_t>(GRID_MARGIN + row * (GRID_HEIGHT + GRID_SPACING)),
                GRID_WIDTH, GRID_HEIGHT};
    }

    void drawGrid(const char* const labels[], int count) {
        screenSprite.fillSprite(TFT_BLACK);
        for (int i = 0; i < count; i++) {
            UIButton button(i, labels[i], gridBounds(i));
            button.draw(screenSprite, 2, 1);
        }
        screenSprite.pushSprite(0, 0);
    }

    int gridHit(int16_t x, int16_t y, int count) const {
        for (int i = 0; i < count; i++) if (gridBounds(i).contains(x, y)) return i;
        return -1;
    }

    bool handleMainMenu(int16_t x, int16_t y) {
        switch (gridHit(x, y, 4)) {
            case 0: state = SETTINGS_MENU; break;
            case 1: state = GAUGE_TYPE_MENU; break;
            case 2: state = COLOR_MENU; break;
            case 3: return false;
            default: return true;
        }
        drawCurrentScreen(); return true;
    }

    bool handleSettingsMenu(int16_t x, int16_t y) {
        switch (gridHit(x, y, 4)) {
            case 0: state = BLUETOOTH_MENU; break;
            case 1: state = GMETER_MENU; break;
            case 2: state = DEVICE_MENU; break;
            case 3: state = MAIN_MENU; break;
            default: return true;
        }
        drawCurrentScreen(); return true;
    }

    void drawBluetoothMenu() {
        const char* labels[] = {"Pair Device", "Stats", "Remove Device", "Exit"};
        drawGrid(labels, 4);
    }

    bool handleBluetoothMenu(int16_t x, int16_t y) {
        int hit = gridHit(x, y, 4);
        if (hit == 1) state = BLUETOOTH_STATS;
        else if (hit == 3) state = SETTINGS_MENU;
        else return true;
        drawCurrentScreen(); return true;
    }

    void drawBluetoothStats() {
        screenSprite.fillSprite(TFT_BLACK);
        UITable table;
        table.configure({5, 5, 310, 190}, 4, 2);
        table.setCell(0, 0, "Metric"); table.setCell(0, 1, "Value");
        table.setCell(1, 0, "Connected"); table.setCell(1, 1, "Yes");
        table.setCell(2, 0, "Device"); table.setCell(2, 1, "OBD BLE");
        table.setCell(3, 0, "Version"); table.setCell(3, 1, SOFTWARE_VERSION);
        table.draw(screenSprite, true);
        UIButton exitButton(0, "Exit", {10, 205, 300, 30});
        exitButton.draw(screenSprite);
        screenSprite.pushSprite(0, 0);
    }

    void drawGMeterMenu() {
        screenSprite.fillSprite(TFT_BLACK);
        screenSprite.setTextFont(2); screenSprite.setTextSize(2); screenSprite.setTextColor(TFT_WHITE);
        screenSprite.setTextDatum(TC_DATUM);
        screenSprite.drawString("G-Meter Calibration", DISPLAY_CENTER_X, 18);
        screenSprite.setTextSize(1);
        screenSprite.drawString("Park on flat ground before calibrating.", DISPLAY_CENTER_X, 52);
        screenSprite.setTextDatum(TL_DATUM);
        UIButton start(0, "Start Calibration", {65, 90, 190, 55}, TFT_DARKGREEN);
        UIButton exit(1, "Exit", {65, 160, 190, 55});
        start.draw(screenSprite); exit.draw(screenSprite);
        screenSprite.pushSprite(0, 0);
    }

    bool handleGMeterMenu(int16_t x, int16_t y) {
        if (UIRect{65, 90, 190, 55}.contains(x, y)) {
            for (int i = 0; i < numGauges; i++) {
                if (gauges[i]->getType() == Gauge::G_METER) {
                    static_cast<GMeter*>(gauges[i])->beginManualCalibration(); break;
                }
            }
        } else if (UIRect{65, 160, 190, 55}.contains(x, y)) {
            state = SETTINGS_MENU;
        }
        drawCurrentScreen(); return true;
    }

    void drawDeviceMenu() {
        screenSprite.fillSprite(TFT_BLACK);
        screenSprite.setTextFont(2); screenSprite.setTextSize(2); screenSprite.setTextColor(TFT_WHITE);
        screenSprite.setTextDatum(MC_DATUM);
        screenSprite.drawString("Device settings", DISPLAY_CENTER_X, 80);
        screenSprite.setTextSize(1);
        screenSprite.drawString("Coming soon", DISPLAY_CENTER_X, 115);
        screenSprite.setTextDatum(TL_DATUM);
        UIButton exit(0, "Exit", {65, 160, 190, 55}); exit.draw(screenSprite);
        screenSprite.pushSprite(0, 0);
    }

    bool handleGaugeTypeMenu(int16_t x, int16_t y) {
        int hit = gridHit(x, y, 3);
        if (hit == 0) {
            state = SINGLE_LIST; listTarget = SINGLE_TARGET; listStart = 0;
        } else if (hit == 1) {
            state = DUAL_MENU;
        } else if (hit == 2) {
            state = MAIN_MENU;
        } else return true;
        drawCurrentScreen(); return true;
    }

    void drawDualMenu() {
        DualGauge* dual = static_cast<DualGauge*>(gauges[1]);
        String left = "Left: " + commands->getCommandLabel(dual->getLeftCommandIndex());
        String right = "Right: " + commands->getCommandLabel(dual->getRightCommandIndex());
        const char* labels[] = {left.c_str(), right.c_str(), "Exit"};
        drawGrid(labels, 3);
    }

    bool handleDualMenu(int16_t x, int16_t y) {
        int hit = gridHit(x, y, 3);
        if (hit == 0 || hit == 1) {
            listTarget = hit == 0 ? DUAL_LEFT_TARGET : DUAL_RIGHT_TARGET;
            state = DUAL_LIST; listStart = 0;
        } else if (hit == 2) state = GAUGE_TYPE_MENU;
        else return true;
        drawCurrentScreen(); return true;
    }

    int currentListSelection() const {
        if (listTarget == SINGLE_TARGET) return static_cast<NeedleGauge*>(gauges[0])->getCommandIndex();
        DualGauge* dual = static_cast<DualGauge*>(gauges[1]);
        return listTarget == DUAL_LEFT_TARGET ? dual->getLeftCommandIndex() : dual->getRightCommandIndex();
    }

    void drawCommandList() {
        screenSprite.fillSprite(TFT_BLACK);
        screenSprite.setTextFont(2); screenSprite.setTextSize(1); screenSprite.setTextColor(TFT_WHITE);
        const char* title = listTarget == SINGLE_TARGET ? "Single Gauge" :
                            (listTarget == DUAL_LEFT_TARGET ? "Dual Gauge - Left" : "Dual Gauge - Right");
        screenSprite.setTextDatum(TC_DATUM); screenSprite.drawString(title, DISPLAY_CENTER_X, 3);
        screenSprite.setTextDatum(ML_DATUM);
        int selected = currentListSelection();
        for (int i = 0; i < LIST_ROWS; i++) {
            int commandIndex = listStart + i;
            if (commandIndex >= commands->getCommandCount()) break;
            int y = LIST_TOP + i * LIST_ROW_HEIGHT;
            uint16_t bg = commandIndex == selected ? TFT_DARKGREEN : TFT_BLACK;
            screenSprite.fillRect(5, y, 310, LIST_ROW_HEIGHT - 2, bg);
            screenSprite.drawRect(5, y, 310, LIST_ROW_HEIGHT - 2, TFT_WHITE);
            String label = commands->getCommandLabel(commandIndex);
            while (label.length() && screenSprite.textWidth(label) > 294) label.remove(label.length() - 1);
            screenSprite.drawString(label, 12, y + ((LIST_ROW_HEIGHT - 2) / 2));
        }
        screenSprite.setTextDatum(TL_DATUM);
        const char* nav[] = {"Prev", "Next", "Exit"};
        for (int i = 0; i < 3; i++) {
            UIButton button(i, nav[i], {static_cast<int16_t>(5 + i * 105), LIST_BUTTON_Y, 100, 36});
            button.draw(screenSprite);
        }
        screenSprite.pushSprite(0, 0);
    }

    bool handleCommandList(int16_t x, int16_t y) {
        if (y >= LIST_BUTTON_Y) {
            int hit = x < 105 ? 0 : (x < 210 ? 1 : 2);
            if (hit == 0) listStart = max(0, listStart - LIST_ROWS);
            else if (hit == 1) listStart = min(max(0, commands->getCommandCount() - LIST_ROWS), listStart + LIST_ROWS);
            else state = listTarget == SINGLE_TARGET ? GAUGE_TYPE_MENU : DUAL_MENU;
            drawCurrentScreen(); return true;
        }
        if (y >= LIST_TOP && y < LIST_TOP + LIST_ROWS * LIST_ROW_HEIGHT) {
            int selected = listStart + ((y - LIST_TOP) / LIST_ROW_HEIGHT);
            if (selected < commands->getCommandCount()) {
                if (listTarget == SINGLE_TARGET) {
                    static_cast<NeedleGauge*>(gauges[0])->setCommandIndex(selected);
                    state = GAUGE_TYPE_MENU;
                } else {
                    DualGauge* dual = static_cast<DualGauge*>(gauges[1]);
                    int left = dual->getLeftCommandIndex();
                    int right = dual->getRightCommandIndex();
                    dual->setCommandIndices(listTarget == DUAL_LEFT_TARGET ? selected : left,
                                            listTarget == DUAL_RIGHT_TARGET ? selected : right);
                    state = DUAL_MENU;
                }
                drawCurrentScreen();
            }
        }
        return true;
    }

    bool handleColorMenu(int16_t x, int16_t y) {
        int hit = gridHit(x, y, 4);
        if (hit >= 0 && hit <= 2) {
            colorTarget = hit == 0 ? VALUE_TARGET : (hit == 1 ? LABEL_TARGET : OUTLINE_TARGET);
            uint16_t current = colorTarget == VALUE_TARGET ? gauges[0]->getCurrentValueColor() :
                               (colorTarget == LABEL_TARGET ? gauges[0]->getCurrentLabelColor() : gauges[0]->getCurrentOutlineColor());
            rgb565ToHsv(current, pickerHue, pickerSaturation, pickerValue);
            pendingColor = current;
            state = COLOR_PICKER;
        } else if (hit == 3) state = MAIN_MENU;
        else return true;
        drawCurrentScreen(); return true;
    }

    uint16_t hsvTo565(float hue, float saturation, float value) const {
        float c = value * saturation;
        float h = hue / 60.0f;
        float x = c * (1.0f - fabsf(fmodf(h, 2.0f) - 1.0f));
        float r = 0, g = 0, b = 0;
        if (h < 1) { r = c; g = x; }
        else if (h < 2) { r = x; g = c; }
        else if (h < 3) { g = c; b = x; }
        else if (h < 4) { g = x; b = c; }
        else if (h < 5) { r = x; b = c; }
        else { r = c; b = x; }
        float m = value - c; r += m; g += m; b += m;
        return static_cast<uint16_t>(((int)(r * 31.0f + 0.5f) << 11) |
                                     ((int)(g * 63.0f + 0.5f) << 5) |
                                     (int)(b * 31.0f + 0.5f));
    }

    void rgb565ToHsv(uint16_t color, float& hue, float& saturation, float& value) const {
        float r = ((color >> 11) & 0x1F) / 31.0f;
        float g = ((color >> 5) & 0x3F) / 63.0f;
        float b = (color & 0x1F) / 31.0f;
        float maxValue = r > g ? (r > b ? r : b) : (g > b ? g : b);
        float minValue = r < g ? (r < b ? r : b) : (g < b ? g : b);
        float delta = maxValue - minValue;
        value = maxValue;
        saturation = maxValue == 0.0f ? 0.0f : delta / maxValue;
        if (delta == 0.0f) hue = 0.0f;
        else if (maxValue == r) hue = 60.0f * fmodf(((g - b) / delta), 6.0f);
        else if (maxValue == g) hue = 60.0f * (((b - r) / delta) + 2.0f);
        else hue = 60.0f * (((r - g) / delta) + 4.0f);
        if (hue < 0.0f) hue += 360.0f;
    }

    void drawColorPicker() {
        const int paletteX = 5, paletteY = 5, paletteW = 170, paletteH = 180;
        const int hueX = 180, hueW = 20;
        screenSprite.fillSprite(TFT_BLACK);
        for (int y = 0; y < paletteH; y++) {
            float val = 1.0f - ((float)y / (paletteH - 1));
            for (int x = 0; x < paletteW; x++) {
                float sat = (float)x / (paletteW - 1);
                screenSprite.drawPixel(paletteX + x, paletteY + y, hsvTo565(pickerHue, sat, val));
            }
        }
        for (int y = 0; y < paletteH; y++) {
            uint16_t color = hsvTo565((float)y * 359.0f / (paletteH - 1), 1.0f, 1.0f);
            screenSprite.drawFastHLine(hueX, paletteY + y, hueW, color);
        }
        int markerX = paletteX + (int)(pickerSaturation * (paletteW - 1));
        int markerY = paletteY + (int)((1.0f - pickerValue) * (paletteH - 1));
        screenSprite.drawCircle(markerX, markerY, 4, TFT_WHITE);
        int hueY = paletteY + (int)(pickerHue * (paletteH - 1) / 359.0f);
        screenSprite.drawRect(hueX - 1, hueY - 2, hueW + 2, 5, TFT_WHITE);
        screenSprite.fillRect(207, 5, 108, 60, pendingColor);
        screenSprite.drawRect(207, 5, 108, 60, TFT_WHITE);
        UIButton confirm(0, "Confirm", {207, 78, 108, 45}, TFT_DARKGREEN);
        UIButton cancel(1, "Cancel", {207, 138, 108, 45});
        confirm.draw(screenSprite); cancel.draw(screenSprite);
        screenSprite.setTextFont(2); screenSprite.setTextSize(1); screenSprite.setTextColor(TFT_WHITE);
        screenSprite.setTextDatum(TC_DATUM);
        screenSprite.drawString("Tap palette, preview, then confirm", DISPLAY_CENTER_X, 205);
        screenSprite.setTextDatum(TL_DATUM);
        screenSprite.pushSprite(0, 0);
    }

    bool handleColorPicker(int16_t x, int16_t y) {
        if (x >= 5 && x < 175 && y >= 5 && y < 185) {
            pickerSaturation = (float)(x - 5) / 169.0f;
            pickerValue = 1.0f - ((float)(y - 5) / 179.0f);
            pendingColor = hsvTo565(pickerHue, pickerSaturation, pickerValue);
        } else if (x >= 180 && x < 200 && y >= 5 && y < 185) {
            pickerHue = (float)(y - 5) * 359.0f / 179.0f;
            pendingColor = hsvTo565(pickerHue, pickerSaturation, pickerValue);
        } else if (UIRect{207, 78, 108, 45}.contains(x, y)) {
            uint16_t label = gauges[0]->getCurrentLabelColor();
            uint16_t value = gauges[0]->getCurrentValueColor();
            uint16_t outline = gauges[0]->getCurrentOutlineColor();
            if (colorTarget == LABEL_TARGET) label = pendingColor;
            else if (colorTarget == VALUE_TARGET) value = pendingColor;
            else outline = pendingColor;
            for (int i = 0; i < numGauges; i++) gauges[i]->setThemeColors(label, value, outline);
            state = COLOR_MENU;
        } else if (UIRect{207, 138, 108, 45}.contains(x, y)) {
            state = COLOR_MENU;
        } else return true;
        drawCurrentScreen(); return true;
    }

    bool navigateBack() {
        switch (state) {
            case MAIN_MENU: return false;
            case SETTINGS_MENU:
            case GAUGE_TYPE_MENU:
            case COLOR_MENU: state = MAIN_MENU; break;
            case BLUETOOTH_MENU:
            case GMETER_MENU:
            case DEVICE_MENU: state = SETTINGS_MENU; break;
            case BLUETOOTH_STATS: state = BLUETOOTH_MENU; break;
            case SINGLE_LIST: state = GAUGE_TYPE_MENU; break;
            case DUAL_MENU: state = GAUGE_TYPE_MENU; break;
            case DUAL_LIST: state = DUAL_MENU; break;
            case COLOR_PICKER: state = COLOR_MENU; break;
        }
        drawCurrentScreen(); return true;
    }
};

#endif
