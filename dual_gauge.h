#ifndef DUAL_GAUGE_H
#define DUAL_GAUGE_H

#include "gauge.h"
#include "commands.h"

class DualGauge : public Gauge {
public:
    DualGauge(TFT_eSPI* display, Commands* commands, int leftCommandIndex, int rightCommandIndex,
              uint32_t outlineColor, uint32_t labelColor, uint32_t valueColor) :
    Gauge(display),
    commands(commands),
    gaugeOutlineLeft(display),
    gaugeOutlineRight(display),
    gaugeMarkerLeft(display),
    gaugeMarkerRight(display),
    gaugeValueLeft(display),
    gaugeValueRight(display),
    gaugeEraserLeft(display),
    gaugeEraserRight(display),
    stats(display),
    valueLabelLeft(display),
    valueLabelRight(display),
    valueUnitsLeft(display),
    valueUnitsRight(display),
    minValueLeft(commands->getCommandMin(leftCommandIndex)),
    minValueRight(commands->getCommandMin(rightCommandIndex)),
    maxValueLeft(commands->getCommandMax(leftCommandIndex)),
    maxValueRight(commands->getCommandMax(rightCommandIndex)),
    leftCommandIndex(leftCommandIndex),
    rightCommandIndex(rightCommandIndex),
    leftDecimals(commands->getCommandDecimals(leftCommandIndex)),
    rightDecimals(commands->getCommandDecimals(rightCommandIndex)),
    targetValueLeft(0.0),
    targetValueRight(0.0),
    labelColor(labelColor),
    outlineColor(outlineColor),
    valueColor(valueColor) {}

    void initialize() override {
      // Clear screen
      display->fillScreen(DISPLAY_BG_COLOR);

      // Initialize both gauges
      createBarGraphs();
      createLabels();
      createValues();
      updateBarGraphs(0.0, 0.0);
      plotValues(0.0, 0.0);
      render(0.0);
    }

    void render(double) override {
      updateBarGraphs(targetValueLeft, targetValueRight);
      plotValues(targetValueLeft, targetValueRight);
      
      //while(true) {
      //  for (int i = 0; i < 450; i++) {
      //    updateBarGraphs((double) i, (double) i);
      //    plotValues((double) i, (double) i);
      //  }
//
      //  for (int i = 450; i > 0; i--) {
      //    updateBarGraphs((double) i, (double) i);
      //    plotValues((double) i, (double) i);
      //  }
      //}
    }

    void reset() {
        display->fillScreen(DISPLAY_BG_COLOR);
        gaugeOutlineLeft.deleteSprite();
        gaugeOutlineRight.deleteSprite();
        gaugeMarkerLeft.deleteSprite();
        gaugeMarkerRight.deleteSprite();
        gaugeValueLeft.deleteSprite();
        gaugeValueRight.deleteSprite();
        gaugeEraserLeft.deleteSprite();
        gaugeEraserRight.deleteSprite();
        valueLabelLeft.deleteSprite();
        valueLabelRight.deleteSprite();
        initialize();
    }

    void displayStats(float fps, double frameAvg, double queryAvg) {

    }

    void setReadings(double leftReading, double rightReading) {
      targetValueLeft = leftReading;
      targetValueRight = rightReading;
    }
    
    void setCommandIndices(int left, int right) {
        if (left >= 0 && left < commands->getCommandCount()) leftCommandIndex = left;
        if (right >= 0 && right < commands->getCommandCount()) rightCommandIndex = right;
        minValueLeft = commands->getCommandMin(leftCommandIndex);
        minValueRight = commands->getCommandMin(rightCommandIndex);
        maxValueLeft = commands->getCommandMax(leftCommandIndex);
        maxValueRight = commands->getCommandMax(rightCommandIndex);
        leftDecimals = commands->getCommandDecimals(leftCommandIndex);
        rightDecimals = commands->getCommandDecimals(rightCommandIndex);
    }

    int getLeftCommandIndex() const { return leftCommandIndex; }
    int getRightCommandIndex() const { return rightCommandIndex; }

    void setThemeColors(uint16_t label, uint16_t value, uint16_t outline) override {
        labelColor = label;
        valueColor = value;
        outlineColor = outline;
    }

    GaugeType getType() const override {
        return DUAL_GAUGE;
    }

    uint32_t getCurrentLabelColor() override { return labelColor; }
    uint32_t getCurrentOutlineColor() override { return outlineColor; }
    uint32_t getCurrentValueColor() override { return valueColor; }

private:
    Commands* commands;
    TFT_eSprite gaugeOutlineLeft, gaugeOutlineRight, gaugeMarkerLeft, gaugeMarkerRight, gaugeValueLeft, gaugeValueRight, gaugeEraserLeft, gaugeEraserRight, valueLabelLeft, valueLabelRight, valueUnitsLeft, valueUnitsRight, stats;
    uint16_t outlineColor, labelColor, valueColor;
    double minValueLeft, minValueRight, maxValueLeft, maxValueRight, targetValueLeft, targetValueRight;
    int leftCommandIndex, rightCommandIndex;
    uint8_t leftDecimals, rightDecimals;

    void createBarGraphs() {
      // Lines for aligment
      //int x = (DISPLAY_WIDTH / 4);
      //int y = DISPLAY_HEIGHT;
      //display->drawLine(x, 0, x, y, TFT_RED);
      //display->drawLine(3*x, 0, 3*x, y, TFT_RED);

      // Create sprites
      if (!gaugeOutlineLeft.createSprite(DG_BAR_WIDTH, DG_BAR_HEIGHT)) {
        Serial.println("Failed to create gaugeOutlineLeft sprite");
      }

      if (!gaugeOutlineRight.createSprite(DG_BAR_WIDTH, DG_BAR_HEIGHT)) {
        Serial.println("Failed to create gaugeOutlineRight sprite");
      }

      gaugeOutlineLeft.fillSprite(DISPLAY_BG_COLOR);
      gaugeOutlineRight.fillSprite(DISPLAY_BG_COLOR);

      // Draw bar graph outlines
      gaugeOutlineLeft.drawRect(0, 0, DG_BAR_WIDTH, DG_BAR_HEIGHT, outlineColor);
      gaugeOutlineRight.drawRect(0, 0, DG_BAR_WIDTH, DG_BAR_HEIGHT, outlineColor);

      // Push sprites at the 1/4 and 3/4 display width
      int xPos = (DISPLAY_WIDTH / 4) - (DG_BAR_WIDTH / 2);
      int yPos = (DISPLAY_HEIGHT / 2) - (DG_BAR_HEIGHT / 2);
      gaugeOutlineLeft.pushSprite(xPos, yPos);

      xPos = 3 * (DISPLAY_WIDTH / 4) - (DG_BAR_WIDTH / 2);
      gaugeOutlineRight.pushSprite(xPos, yPos);
    }

    void updateBarGraphs(double leftVal, double rightVal) {
      gaugeOutlineLeft.fillRect(1, 1, DG_BAR_WIDTH - 2, DG_BAR_HEIGHT - 2, valueColor);
      gaugeOutlineRight.fillRect(1, 1, DG_BAR_WIDTH - 2, DG_BAR_HEIGHT - 2, valueColor);

      // Fill bar from top down with black to (1 - (value / maxValue)) * DG_BAR_HEIGHT
      leftVal = constrain(leftVal, minValueLeft, maxValueLeft);
      rightVal = constrain(rightVal, minValueRight, maxValueRight);

      double leftRange = maxValueLeft - minValueLeft;
      double rightRange = maxValueRight - minValueRight;
      int y = (1.0 - ((leftVal - minValueLeft) / (leftRange > 0.0 ? leftRange : 1.0))) * (DG_BAR_HEIGHT - 2);
      if (y != 0) {
        // Only render black if bar graph is not full
        gaugeOutlineLeft.fillRect(1, 1, DG_BAR_WIDTH - 2, y - 2, DISPLAY_BG_COLOR);
      }

      y = (1.0 - ((rightVal - minValueRight) / (rightRange > 0.0 ? rightRange : 1.0))) * (DG_BAR_HEIGHT - 2);
      if (y != 0) {
        // Only render black if bar graph is not full
        gaugeOutlineRight.fillRect(1, 1, DG_BAR_WIDTH - 2, y - 2, DISPLAY_BG_COLOR);
      }

      // Plot outlines
      int xPos = (DISPLAY_WIDTH / 4) - (DG_BAR_WIDTH / 2);
      int yPos = (DISPLAY_HEIGHT / 2) - (DG_BAR_HEIGHT / 2);
      gaugeOutlineLeft.pushSprite(xPos, yPos);

      xPos = 3 * (DISPLAY_WIDTH / 4) - (DG_BAR_WIDTH / 2);
      gaugeOutlineRight.pushSprite(xPos, yPos);
    }

    void createLabels() {
      const int columnWidth = DISPLAY_WIDTH / 2;
      const int textWidth = columnWidth - 10;
      String leftLabel = commands->getCommandLabel(leftCommandIndex);
      String rightLabel = commands->getCommandLabel(rightCommandIndex);
      String leftUnits = commands->getCommandUnits(leftCommandIndex);
      String rightUnits = commands->getCommandUnits(rightCommandIndex);

      valueLabelLeft.deleteSprite(); valueLabelRight.deleteSprite();
      valueUnitsLeft.deleteSprite(); valueUnitsRight.deleteSprite();
      valueLabelLeft.setFreeFont(FONT_BOLD_10);
      valueLabelRight.setFreeFont(FONT_BOLD_10);
      valueLabelLeft.createSprite(textWidth, 24);
      valueLabelRight.createSprite(textWidth, 24);
      valueLabelLeft.fillSprite(DISPLAY_BG_COLOR);
      valueLabelRight.fillSprite(DISPLAY_BG_COLOR);
      valueLabelLeft.setTextColor(labelColor, DISPLAY_BG_COLOR);
      valueLabelRight.setTextColor(labelColor, DISPLAY_BG_COLOR);
      while (leftLabel.length() && valueLabelLeft.textWidth(leftLabel) > textWidth - 4) leftLabel.remove(leftLabel.length() - 1);
      while (rightLabel.length() && valueLabelRight.textWidth(rightLabel) > textWidth - 4) rightLabel.remove(rightLabel.length() - 1);
      valueLabelLeft.setTextDatum(MC_DATUM);
      valueLabelRight.setTextDatum(MC_DATUM);
      valueLabelLeft.drawString(leftLabel, textWidth / 2, 12);
      valueLabelRight.drawString(rightLabel, textWidth / 2, 12);

      valueUnitsLeft.setFreeFont(FONT_NORMAL_8);
      valueUnitsRight.setFreeFont(FONT_NORMAL_8);
      valueUnitsLeft.createSprite(textWidth, 18);
      valueUnitsRight.createSprite(textWidth, 18);
      valueUnitsLeft.fillSprite(DISPLAY_BG_COLOR);
      valueUnitsRight.fillSprite(DISPLAY_BG_COLOR);
      valueUnitsLeft.setTextColor(labelColor, DISPLAY_BG_COLOR);
      valueUnitsRight.setTextColor(labelColor, DISPLAY_BG_COLOR);
      valueUnitsLeft.setTextDatum(MC_DATUM);
      valueUnitsRight.setTextDatum(MC_DATUM);
      valueUnitsLeft.drawString(leftUnits, textWidth / 2, 9);
      valueUnitsRight.drawString(rightUnits, textWidth / 2, 9);

      valueLabelLeft.pushSprite(5, 2);
      valueLabelRight.pushSprite(columnWidth + 5, 2);
      valueUnitsLeft.pushSprite(5, 25);
      valueUnitsRight.pushSprite(columnWidth + 5, 25);
      valueLabelLeft.unloadFont();
      valueLabelRight.unloadFont();
      valueUnitsLeft.unloadFont();
      valueUnitsRight.unloadFont();
    }

    void createValues() {
      // Create sprite, do not push yet
      gaugeValueLeft.deleteSprite();
      gaugeValueRight.deleteSprite();
      gaugeValueLeft.setFreeFont(FONT_BOLD_18);
      int h = gaugeValueLeft.fontHeight();
      if (!gaugeValueLeft.createSprite(VALUE_WIDTH, h)) {
        Serial.println("Could not create gaugeValueLabelLeft sprite");
      }

      gaugeValueRight.setFreeFont(FONT_BOLD_18);
      if (!gaugeValueRight.createSprite(VALUE_WIDTH, h)) {
        Serial.println("Could not create gaugeValueRight sprite");
      }
    }

    void plotValues(double leftVal, double rightVal) {
        // Fill sprites, set fonts and text colors
        gaugeValueLeft.fillSprite(VALUE_BG_COLOR);
        gaugeValueRight.fillSprite(VALUE_BG_COLOR);
        gaugeValueLeft.setFreeFont(FONT_BOLD_18);
        gaugeValueRight.setFreeFont(FONT_BOLD_18);
        gaugeValueLeft.setTextColor(valueColor, DISPLAY_BG_COLOR);
        gaugeValueRight.setTextColor(valueColor, DISPLAY_BG_COLOR);

        // Draw text at the center of each sprite
        int textWidth = 0;
        if (leftDecimals == 0) {
            int intVal = (int)round(leftVal);
            textWidth = gaugeValueLeft.textWidth(String(intVal));
            int x = (VALUE_WIDTH - textWidth) / 2;
            gaugeValueLeft.drawNumber(intVal, x, 0);
        } else {
            textWidth = gaugeValueLeft.textWidth(String(leftVal, static_cast<unsigned int>(leftDecimals)));
            int x = (VALUE_WIDTH - textWidth) / 2;
            gaugeValueLeft.drawFloat(leftVal, leftDecimals, x, 0);
        }

        if (rightDecimals == 0) {
            int intVal = (int)round(rightVal);
            textWidth = gaugeValueRight.textWidth(String(intVal));
            int x = (VALUE_WIDTH - textWidth) / 2;
            gaugeValueRight.drawNumber(intVal, x, 0);
        } else {
            textWidth = gaugeValueRight.textWidth(String(rightVal, static_cast<unsigned int>(rightDecimals)));
            int x = (VALUE_WIDTH - textWidth) / 2;
            gaugeValueRight.drawFloat(rightVal, rightDecimals, x, 0);
        }

        int xPos = (DISPLAY_WIDTH / 4)  - (VALUE_WIDTH / 2);
        int yPos = (DISPLAY_HEIGHT / 2) + (DG_BAR_HEIGHT / 2) + 10;
        gaugeValueLeft.pushSprite(xPos, yPos);

        xPos = 3 * (DISPLAY_WIDTH / 4)  - (VALUE_WIDTH / 2);
        gaugeValueRight.pushSprite(xPos, yPos);
        gaugeValueLeft.unloadFont();
        gaugeValueRight.unloadFont();
    }
};

#endif




