#ifndef DUAL_GAUGE_H
#define DUAL_GAUGE_H

#include "gauge.h"

class DualGauge : public Gauge {
public:
    DualGauge(TFT_eSPI* display, int gaugeType, uint32_t outlineColor, uint32_t needleColor, uint32_t valueColor) : 
    Gauge(display),
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
    minValueLeft(gaugeTypes[gaugeType][2].toDouble()),
    minValueRight(gaugeTypes[gaugeType + 1][2].toDouble()),
    maxValueLeft(gaugeTypes[gaugeType][3].toDouble()),
    maxValueRight(gaugeTypes[gaugeType + 1][3].toDouble()),
    valueTypeLeft(gaugeTypes[gaugeType][4]),
    valueTypeRight(gaugeTypes[gaugeType + 1][4]),
    targetValueLeft(0.0),
    targetValueRight(0.0),
    needleColor(needleColor),
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
    
    void setNeedleColor(uint16_t color) {
        needleColor = color;
        // Recreate needle sprite with new color
        gaugeOutlineLeft.deleteSprite();
        gaugeOutlineRight.deleteSprite();
        createBarGraphs();
        updateBarGraphs(targetValueLeft, targetValueRight);
    }

    void setOutlineColor(uint16_t color) {
        outlineColor = color;
        gaugeOutlineLeft.deleteSprite();
        gaugeOutlineRight.deleteSprite();
        createBarGraphs();
        updateBarGraphs(targetValueLeft, targetValueRight);
    }

    void setValueColor(uint16_t color) {
        valueColor = color;
        gaugeValueLeft.deleteSprite();
        gaugeValueRight.deleteSprite();
        valueLabelLeft.deleteSprite();
        valueLabelRight.deleteSprite();
        createLabels();
        createValues();
        plotValues(0.0, 0.0);
    }

    GaugeType getType() const override {
        return DUAL_GAUGE;
    }

    uint32_t getCurrentNeedleColor() {
        return needleColor;
    }

    uint32_t getCurrentOutlineColor() {
        return outlineColor;
    }

    uint32_t getCurrentValueColor() {
        return valueColor;
    }

private:

    TFT_eSprite gaugeOutlineLeft, gaugeOutlineRight, gaugeMarkerLeft, gaugeMarkerRight, gaugeValueLeft, gaugeValueRight, gaugeEraserLeft, gaugeEraserRight, valueLabelLeft, valueLabelRight, valueUnitsLeft, valueUnitsRight, stats;
    uint16_t outlineColor, needleColor, valueColor;
    double minValueLeft, minValueRight, maxValueLeft, maxValueRight, targetValueLeft, targetValueRight;
    String valueTypeLeft, valueTypeRight;
    static String gaugeTypes[2][5];

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

      if (!gaugeOutlineRight.createSprite(DG_BAR_WIDTH, DG_BAR_WIDTH)) {
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
      // Clear bars by filling with RED
      gaugeOutlineLeft.fillRect(1, 1, DG_BAR_WIDTH - 2, DG_BAR_HEIGHT - 2, needleColor);
      gaugeOutlineRight.fillRect(1, 1, DG_BAR_WIDTH - 2, DG_BAR_HEIGHT - 2, needleColor);

      // Fill bar from top down with black to (1 - (value / maxValue)) * DG_BAR_HEIGHT
      int y = (1 - (leftVal / maxValueLeft)) * DG_BAR_HEIGHT;
      if (y != 0) {
        // Only render black if bar graph is not full
        gaugeOutlineLeft.fillRect(1, 1, DG_BAR_WIDTH - 2, y - 2, DISPLAY_BG_COLOR);
      }

      y = (1 - (rightVal / maxValueRight)) * DG_BAR_HEIGHT;
      if (y != 0) {
        // Only render black if bar graph is not full
        gaugeOutlineRight.fillRect(1, 1, DG_BAR_WIDTH - 2, y - 2, DISPLAY_BG_COLOR);
      }

      // Plot outlines
      int xPos = (DISPLAY_WIDTH / 4) - (DG_BAR_WIDTH / 2);
      int yPos = (DISPLAY_HEIGHT / 2) - (DG_BAR_HEIGHT / 2);
      gaugeOutlineLeft.pushSprite(xPos, yPos);

      xPos = 3 * (DISPLAY_WIDTH / 4) - (DG_BAR_WIDTH / 2);
      gaugeOutlineLeft.pushSprite(xPos, yPos);
    }

    void createLabels() {
      // Create left label
      String leftLabel = String(gaugeTypes[0][0]);
      valueLabelLeft.setFreeFont(FONT_BOLD_16);
      valueLabelLeft.setTextColor(valueColor);
      int w = valueLabelLeft.textWidth(leftLabel);
      int h = valueLabelLeft.fontHeight();
      if (!valueLabelLeft.createSprite(w, h)) {
        Serial.println("Could not create valueLabelLeft sprite");
      }
      valueLabelLeft.drawString(leftLabel, 0, 0);

      // Create right label
      String rightLabel = String(gaugeTypes[1][0]);
      valueLabelRight.setFreeFont(FONT_BOLD_16);
      valueLabelRight.setTextColor(valueColor);
      w = valueLabelRight.textWidth(rightLabel);
      h = valueLabelRight.fontHeight();
      if (!valueLabelRight.createSprite(w, h)) {
        Serial.println("Could not create valueLabelRight sprite");
      }
      valueLabelRight.drawString(rightLabel, 0, 0);

      // Push sprites an unload font
      int xPos = (DISPLAY_WIDTH / 4)  - (w / 2);
      int yPos = (DISPLAY_HEIGHT / 2) - (DG_BAR_HEIGHT / 2) - h;
      valueLabelLeft.pushSprite(xPos, yPos);

      xPos = 3 * (DISPLAY_WIDTH / 4)  - (w / 2);
      valueLabelRight.pushSprite(xPos, yPos);
      valueLabelLeft.unloadFont();
      valueLabelRight.unloadFont();
    }

    void createValues() {
      // Create sprite, do not push yet
      gaugeValueLeft.setFreeFont(FONT_BOLD_18);
      int w = gaugeValueLeft.textWidth(String(maxValueLeft));
      int h = gaugeValueLeft.fontHeight();
      if (!gaugeValueLeft.createSprite(w, h)) {
        Serial.println("Could not create gaugeValueLabelLeft sprite");
      }

      gaugeValueRight.setFreeFont(FONT_BOLD_18);
      w = gaugeValueRight.textWidth(String(maxValueRight));
      h = gaugeValueRight.fontHeight();
      if (!gaugeValueRight.createSprite(w, h)) {
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
        if (valueTypeLeft == "int") {
            int intVal = (int)round(leftVal);
            textWidth = gaugeValueLeft.textWidth(String(intVal));
            int x = (VALUE_WIDTH - textWidth) / 2;
            gaugeValueLeft.drawNumber(intVal, x, 0);
        } else {
            textWidth = gaugeValueLeft.textWidth(String(leftVal, 1));
            int x = (VALUE_WIDTH - textWidth) / 2;
            gaugeValueLeft.drawFloat(leftVal, 1, x, 0);
        }

        if (valueTypeRight == "int") {
            int intVal = (int)round(rightVal);
            textWidth = gaugeValueRight.textWidth(String(intVal));
            int x = (VALUE_WIDTH - textWidth) / 2;
            gaugeValueRight.drawNumber(intVal, x, 0);
        } else {
            textWidth = gaugeValueRight.textWidth(String(rightVal, 1));
            int x = (VALUE_WIDTH - textWidth) / 2;
            gaugeValueRight.drawFloat(rightVal, 1, x, 0);
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

// Define and initialize static member outside class
String DualGauge::gaugeTypes[2][5] = {
    {"TORQUE", "lb-ft", "0", "445", "int"},
    {"POWER", "hp", "0", "450", "int"}
};

#endif





