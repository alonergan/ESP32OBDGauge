#ifndef QUADRANT_GAUGE_H
#define QUADRANT_GAUGE_H

#include "gauge.h"
#include "commands.h"
#include "ui_library.h"

class QuadrantGauge : public Gauge {
public:
    QuadrantGauge(TFT_eSPI* display, Commands* commands, const int commandIndices[4],
                  uint16_t outlineColor, uint16_t labelColor, uint16_t valueColor) :
        Gauge(display),
        commands(commands),
        screen(display),
        selector(display),
        selectorOpen(false),
        selectedQuadrant(0),
        pageStart(0),
        lastSelectorButtonPressMs(0),
        lastSelectorButtonId(-1),
        outlineColor(outlineColor),
        labelColor(labelColor),
        valueColor(valueColor) {
        for (int i = 0; i < 4; i++) {
            selectedCommands[i] = commandIndices[i];
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

    void setThemeColors(uint16_t label, uint16_t value, uint16_t outline) override {
        labelColor = label;
        valueColor = value;
        outlineColor = outline;
    }
    uint32_t getCurrentLabelColor() override { return labelColor; }
    uint32_t getCurrentOutlineColor() override { return outlineColor; }
    uint32_t getCurrentValueColor() override { return valueColor; }

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

        const int rowHeight = 32;
        const int top = 24;
        const int bottomButtonY = 198;
        const int visibleRows = 5;

        if (localY >= bottomButtonY && localY < bottomButtonY + 36) {
            int buttonId = localX < 105 ? 0 : (localX < 210 ? 1 : 2);
            if (!shouldProcessSelectorButton(buttonId)) return true;
            if (buttonId == 0) pageStart = max(0, pageStart - visibleRows);
            else if (buttonId == 1) pageStart = min(max(0, commands->getCommandCount() - visibleRows), pageStart + visibleRows);
            else { selectorOpen = false; drawQuadrants(); }
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
    uint16_t outlineColor;
    uint16_t labelColor;
    uint16_t valueColor;

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

    void drawCenteredFittedText(const String& text, int centerX, int y, int maxWidth) {
        String fitted = fitTextToWidth(text, maxWidth);
        int fittedWidth = screen.textWidth(fitted);
        screen.drawString(fitted, centerX - (fittedWidth / 2), y);
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
        screen.drawLine(DISPLAY_WIDTH / 2, 0, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT, outlineColor);
        screen.drawLine(0, DISPLAY_HEIGHT / 2, DISPLAY_WIDTH, DISPLAY_HEIGHT / 2, outlineColor);

        for (int i = 0; i < 4; i++) {
            int originX = (i % 2) * (DISPLAY_WIDTH / 2);
            int originY = (i / 2) * (DISPLAY_HEIGHT / 2);
            int boxW = DISPLAY_WIDTH / 2;
            int boxH = DISPLAY_HEIGHT / 2;

            String label = commands->getCommandLabel(selectedCommands[i]);
            String units = commands->getCommandUnits(selectedCommands[i]);

            screen.setTextColor(labelColor, DISPLAY_BG_COLOR);
            const int textMaxWidth = boxW - 16;
            const int textCenterX = originX + (boxW / 2);

            screen.setFreeFont(FONT_BOLD_8);
            drawCenteredFittedText(label, textCenterX, originY + 8, textMaxWidth);
            screen.unloadFont();

            screen.setFreeFont(FONT_BOLD_22);
            String valueText = String(values[i], 1);
            screen.setTextColor(valueColor, DISPLAY_BG_COLOR);
            screen.setTextDatum(MC_DATUM);
            screen.drawString(valueText, textCenterX, originY + (boxH / 2));
            screen.setTextDatum(TL_DATUM);
            screen.unloadFont();

            screen.setFreeFont(FONT_NORMAL_8);
            screen.setTextColor(labelColor, DISPLAY_BG_COLOR);
            drawCenteredFittedText(units, textCenterX, originY + boxH - 24, textMaxWidth);
            screen.unloadFont();
        }

        screen.pushSprite(0, 0);
    }

    void drawSelector() {
        selector.fillSprite(TFT_BLACK);
        selector.setTextColor(TFT_WHITE);
        selector.setTextFont(2);
        selector.setTextSize(1);

        const int rowHeight = 32;
        const int top = 24;
        const int bottomButtonY = 198;
        const int visibleRows = 5;
        selector.setTextDatum(TC_DATUM);
        selector.drawString("Quadrant " + String(selectedQuadrant + 1), DISPLAY_CENTER_X, 3);
        selector.setTextDatum(ML_DATUM);

        for (int i = 0; i < visibleRows; i++) {
            int commandIndex = pageStart + i;
            if (commandIndex >= commands->getCommandCount()) {
                break;
            }

            int rowY = top + i * rowHeight;
            bool active = (selectedCommands[selectedQuadrant] == commandIndex);
            uint16_t bg = active ? TFT_DARKGREEN : TFT_BLACK;
            selector.fillRect(5, rowY, 310, rowHeight - 2, bg);
            selector.drawRect(5, rowY, 310, rowHeight - 2, TFT_WHITE);

            String originalLabel = commands->getCommandLabel(commandIndex);
            String rowLabel = originalLabel;
            const int rowTextMaxWidth = 294;
            if (selector.textWidth(originalLabel) > rowTextMaxWidth) {
                while (rowLabel.length() > 0 && selector.textWidth(rowLabel + "...") > rowTextMaxWidth) {
                    rowLabel.remove(rowLabel.length() - 1);
                }
                rowLabel += "...";
            }
            selector.setTextDatum(ML_DATUM);
            selector.drawString(rowLabel, 12, rowY + ((rowHeight - 2) / 2));
        }

        selector.setTextDatum(TL_DATUM);
        const char* nav[] = {"Prev", "Next", "Exit"};
        for (int i = 0; i < 3; i++) {
            UIButton button(i, nav[i], {static_cast<int16_t>(5 + i * 105), bottomButtonY, 100, 36});
            button.draw(selector);
        }
        selector.pushSprite(0, 0);
    }
};

#endif
