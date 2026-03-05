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
        pageStart(0) {
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

        if (!selector.createSprite(DISPLAY_WIDTH - 30, DISPLAY_HEIGHT - 30)) {
            Serial.println("Failed to create QuadrantGauge selector sprite");
        }

        drawQuadrants();
    }

    void render(double) override {
        drawQuadrants();
        if (selectorOpen) {
            drawSelector();
        }
    }

    void reset() override {
        selectorOpen = false;
        pageStart = 0;
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
        selectorOpen = true;
        drawSelector();
    }

    bool handleTouch(uint16_t x, uint16_t y) {
        if (!selectorOpen) {
            return false;
        }

        int localX = x - 15;
        int localY = y - 15;
        if (localX < 0 || localY < 0 || localX >= (DISPLAY_WIDTH - 30) || localY >= (DISPLAY_HEIGHT - 30)) {
            selectorOpen = false;
            return true;
        }

        const int rowHeight = 24;
        const int top = 32;
        const int visibleRows = 7;

        int prevY = DISPLAY_HEIGHT - 65;
        int nextY = DISPLAY_HEIGHT - 65;
        if (localY >= prevY && localY < prevY + 22 && localX >= 10 && localX < 90) {
            pageStart = max(0, pageStart - visibleRows);
            return true;
        }

        if (localY >= nextY && localY < nextY + 22 && localX >= (DISPLAY_WIDTH - 30 - 90) && localX < (DISPLAY_WIDTH - 30 - 10)) {
            int maxStart = max(0, commands->getCommandCount() - visibleRows);
            pageStart = min(maxStart, pageStart + visibleRows);
            return true;
        }

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

        screen.setTextColor(TFT_YELLOW, DISPLAY_BG_COLOR);
        screen.setTextFont(1);
        String diag = commands->getLastQueryDiagnostic();
        if (diag.length() > 52) {
            diag = diag.substring(0, 52) + "...";
        }
        screen.drawString(diag, 2, DISPLAY_HEIGHT - 10, 1);

        screen.pushSprite(0, 0);
    }

    void drawSelector() {
        selector.fillSprite(TFT_BLACK);
        selector.drawRect(0, 0, selector.width(), selector.height(), TFT_WHITE);
        selector.setTextColor(TFT_WHITE, TFT_BLACK);
        selector.setTextFont(1);

        String title = "Select value for quadrant " + String(selectedQuadrant + 1);
        selector.drawString(title, 8, 8, 2);

        const int rowHeight = 24;
        const int top = 32;
        const int visibleRows = 7;

        for (int i = 0; i < visibleRows; i++) {
            int commandIndex = pageStart + i;
            if (commandIndex >= commands->getCommandCount()) {
                break;
            }

            int rowY = top + i * rowHeight;
            bool active = (selectedCommands[selectedQuadrant] == commandIndex);
            uint16_t bg = active ? TFT_DARKGREEN : TFT_DARKGREY;
            selector.fillRect(6, rowY, selector.width() - 12, rowHeight - 2, bg);

            String originalLabel = commands->getCommandLabel(commandIndex);
            String rowLabel = originalLabel;
            while (rowLabel.length() > 0 && selector.textWidth(rowLabel, 2) > selector.width() - 24) {
                rowLabel.remove(rowLabel.length() - 1);
            }
            if (rowLabel != originalLabel) {
                rowLabel += "...";
            }
            selector.drawString(rowLabel, 12, rowY + 6, 2);
        }

        selector.fillRect(10, selector.height() - 35, 80, 22, TFT_BLUE);
        selector.drawString("Prev", 28, selector.height() - 30, 2);

        selector.fillRect(selector.width() - 90, selector.height() - 35, 80, 22, TFT_BLUE);
        selector.drawString("Next", selector.width() - 72, selector.height() - 30, 2);

        selector.pushSprite(15, 15);
    }
};

#endif
