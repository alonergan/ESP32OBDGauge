#ifndef QUADRANT_GAUGE_H
#define QUADRANT_GAUGE_H

#include "gauge.h"
#include "commands.h"

class QuadrantGauge : public Gauge {
public:
    QuadrantGauge(TFT_eSPI* display, Commands* commands) :
        Gauge(display),
        commands(commands),
        screen(display),
        selector(display),
        selectorOpen(false),
        selectedQuadrant(0),
        pageStart(0),
        lastSelectorButtonPressMs(0),
        lastSelectorButtonId(-1) {
        selectedCommands[0] = 8;   // RPM
        selectedCommands[1] = 9;   // Speed
        selectedCommands[2] = 1;   // Coolant temp
        selectedCommands[3] = 29;  // Fuel rate
        for (int i = 0; i < 4; i++) {
            values[i] = 0.0;
        }
    }

    void initialize() override {
        display->fillScreen(DISPLAY_BG_COLOR);
        if (!screen.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT)) {
            Serial.println("Failed to create QuadrantGauge screen sprite");
        }

        if (!selector.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT)) {
            Serial.println("Failed to create QuadrantGauge selector sprite");
        }

        drawQuadrants();
    }

    void render(double) override {
        if (selectorOpen) {
            drawSelector();
            return;
        }
        drawQuadrants();
    }

    void reset() override {
        selectorOpen = false;
        pageStart = 0;
        clearSelectorButtonDebounce();
        drawQuadrants();
    }

    void displayStats(float, double, double) override {}

    GaugeType getType() const override {
        return QUADRANT_GAUGE;
    }

    uint32_t getCurrentNeedleColor() override { return TFT_WHITE; }
    uint32_t getCurrentOutlineColor() override { return TFT_WHITE; }
    uint32_t getCurrentValueColor() override { return TFT_WHITE; }

    void setQuadrantReading(int quadrant, double value) {
        if (quadrant < 0 || quadrant >= 4) {
            return;
        }
        values[quadrant] = value;
    }

    bool isSelectorVisible() const {
        return selectorOpen;
    }

    void openSelectorFromTouch(uint16_t x, uint16_t y) {
        selectedQuadrant = getQuadrantFromTouch(x, y);
        pageStart = 0;
        clearSelectorButtonDebounce();
        selectorOpen = true;
        drawSelector();
    }

    bool handleTouch(uint16_t x, uint16_t y) {
        if (!selectorOpen) {
            return false;
        }

        int localX = x;
        int localY = y;

        const int rowHeight = 36;
        const int top = 8;
        const int bottomButtonY = DISPLAY_HEIGHT - 44;
        const int visibleRows = (bottomButtonY - top) / rowHeight;

        if (localY >= bottomButtonY && localY < bottomButtonY + 34 && localX >= 10 && localX < 145) {
            if (!shouldProcessSelectorButton(0)) {
                return true;
            }
            pageStart = max(0, pageStart - visibleRows);
            return true;
        }

        if (localY >= bottomButtonY && localY < bottomButtonY + 34 && localX >= (DISPLAY_WIDTH - 145) && localX < (DISPLAY_WIDTH - 10)) {
            if (!shouldProcessSelectorButton(1)) {
                return true;
            }
            int maxStart = max(0, commands->getCommandCount() - visibleRows);
            pageStart = min(maxStart, pageStart + visibleRows);
            return true;
        }

        clearSelectorButtonDebounce();

        for (int i = 0; i < visibleRows; i++) {
            int rowY = top + i * rowHeight;
            if (localY >= rowY && localY < rowY + rowHeight - 2) {
                int commandIndex = pageStart + i;
                if (commandIndex < commands->getCommandCount()) {
                    selectedCommands[selectedQuadrant] = commandIndex;
                    selectorOpen = false;
                }
                return true;
            }
        }

        return true;
    }

    int getCommandIndexForQuadrant(int quadrant) const {
        if (quadrant < 0 || quadrant >= 4) {
            return 0;
        }
        return selectedCommands[quadrant];
    }

private:
    Commands* commands;
    TFT_eSprite screen;
    TFT_eSprite selector;
    int selectedCommands[4];
    double values[4];
    bool selectorOpen;
    int selectedQuadrant;
    int pageStart;
    uint32_t lastSelectorButtonPressMs;
    int lastSelectorButtonId;

    static constexpr uint32_t SELECTOR_BUTTON_DEBOUNCE_MS = 250;

    void clearSelectorButtonDebounce() {
        lastSelectorButtonId = -1;
        lastSelectorButtonPressMs = 0;
    }

    bool shouldProcessSelectorButton(int buttonId) {
        uint32_t now = millis();
        if (buttonId == lastSelectorButtonId && (now - lastSelectorButtonPressMs) < SELECTOR_BUTTON_DEBOUNCE_MS) {
            return false;
        }

        lastSelectorButtonId = buttonId;
        lastSelectorButtonPressMs = now;
        return true;
    }

    String fitTextToWidth(const String& text, int maxWidth) {
        if (screen.textWidth(text) <= maxWidth) {
            return text;
        }

        String fitted = text;
        while (fitted.length() > 0 && screen.textWidth(fitted + "...") > maxWidth) {
            fitted.remove(fitted.length() - 1);
        }

        return fitted + "...";
    }

    void drawFittedText(const String& text, int x, int y, int maxWidth) {
        String fitted = fitTextToWidth(text, maxWidth);
        screen.drawString(fitted, x, y);
    }

    int getQuadrantFromTouch(uint16_t x, uint16_t y) {
        bool right = x >= DISPLAY_WIDTH / 2;
        bool lower = y >= DISPLAY_HEIGHT / 2;

        if (!right && !lower) return 0;
        if (right && !lower) return 1;
        if (!right && lower) return 2;
        return 3;
    }

    void drawQuadrants() {
        screen.fillSprite(DISPLAY_BG_COLOR);
        screen.drawLine(DISPLAY_WIDTH / 2, 0, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT, TFT_DARKGREY);
        screen.drawLine(0, DISPLAY_HEIGHT / 2, DISPLAY_WIDTH, DISPLAY_HEIGHT / 2, TFT_DARKGREY);

        for (int i = 0; i < 4; i++) {
            int originX = (i % 2) * (DISPLAY_WIDTH / 2);
            int originY = (i / 2) * (DISPLAY_HEIGHT / 2);
            int boxW = DISPLAY_WIDTH / 2;
            int boxH = DISPLAY_HEIGHT / 2;

            String label = commands->getCommandLabel(selectedCommands[i]);
            String units = commands->getCommandUnits(selectedCommands[i]);

            screen.setTextColor(TFT_WHITE, DISPLAY_BG_COLOR);
            const int textLeft = originX + 8;
            const int textMaxWidth = boxW - 16;

            screen.setFreeFont(FONT_BOLD_12);
            drawFittedText(label, textLeft, originY + 8, textMaxWidth);
            screen.unloadFont();

            screen.setFreeFont(FONT_BOLD_18);
            String valueText = String(values[i], 1);
            int valueWidth = screen.textWidth(valueText);
            screen.drawString(valueText, originX + (boxW - valueWidth) / 2, originY + (boxH / 2) - 10);
            screen.unloadFont();

            screen.setFreeFont(FONT_NORMAL_12);
            drawFittedText(units, textLeft, originY + boxH - 22, textMaxWidth);
            screen.unloadFont();
        }

        screen.pushSprite(0, 0);
    }

    void drawSelector() {
        selector.fillSprite(TFT_BLACK);
        selector.drawRect(0, 0, selector.width(), selector.height(), TFT_WHITE);
        selector.setTextColor(TFT_WHITE, TFT_BLACK);
        selector.setTextFont(1);

        const int rowHeight = 36;
        const int top = 8;
        const int bottomButtonY = selector.height() - 44;
        const int visibleRows = (bottomButtonY - top) / rowHeight;

        for (int i = 0; i < visibleRows; i++) {
            int commandIndex = pageStart + i;
            if (commandIndex >= commands->getCommandCount()) {
                break;
            }

            int rowY = top + i * rowHeight;
            bool active = (selectedCommands[selectedQuadrant] == commandIndex);
            uint16_t bg = active ? TFT_DARKGREEN : TFT_DARKGREY;
            selector.fillRect(6, rowY, selector.width() - 12, rowHeight - 3, bg);
            selector.drawRect(6, rowY, selector.width() - 12, rowHeight - 3, TFT_WHITE);

            String originalLabel = commands->getCommandLabel(commandIndex);
            String rowLabel = originalLabel;
            while (rowLabel.length() > 0 && selector.textWidth(rowLabel, 2) > selector.width() - 28) {
                rowLabel.remove(rowLabel.length() - 1);
            }
            if (rowLabel != originalLabel) {
                rowLabel += "...";
            }
            selector.drawString(rowLabel, 12, rowY + 9, 2);
        }

        selector.fillRect(10, bottomButtonY, 135, 34, TFT_DARKGREY);
        selector.drawRect(10, bottomButtonY, 135, 34, TFT_WHITE);
        selector.drawString("Prev", 54, bottomButtonY + 10, 2);

        selector.fillRect(selector.width() - 145, bottomButtonY, 135, 34, TFT_DARKGREY);
        selector.drawRect(selector.width() - 145, bottomButtonY, 135, 34, TFT_WHITE);
        selector.drawString("Next", selector.width() - 100, bottomButtonY + 10, 2);

        selector.pushSprite(0, 0);
    }
};

#endif
