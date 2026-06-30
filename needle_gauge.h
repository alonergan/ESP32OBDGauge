#ifndef NEEDLE_GAUGE_H
#define NEEDLE_GAUGE_H

#include "gauge.h"
#include "commands.h"
#include "ui_library.h"

class NeedleGauge : public Gauge {
public:
    NeedleGauge(TFT_eSPI* display, Commands* commands, int commandIndex,
                uint32_t outlineColor, uint32_t labelColor, uint32_t valueColor) :
        Gauge(display),
        commands(commands),
        gaugeOutline(display),
        gaugeNeedle(display),
        gaugeValue(display),
        gaugeEraser(display),
        gaugeTicks(display),
        stats(display),
        valueLabel(commands->getCommandLabel(commandIndex)),
        valueUnits(commands->getCommandUnits(commandIndex)),
        minValue(commands->getCommandMin(commandIndex)),
        maxValue(commands->getCommandMax(commandIndex)),
        commandIndex(commandIndex),
        valueDecimals(commands->getCommandDecimals(commandIndex)),
        targetValue(0.0),
        currentAngle(GAUGE_START_ANGLE),
        oldAngle(GAUGE_START_ANGLE),
        sweepState(SWEEP_UP),
        sweepStartTime(0),
        sweepValue(0.0),
        labelColor(labelColor),
        outlineColor(outlineColor),
        valueColor(valueColor) {}

    void initialize() override {
        display->fillScreen(DISPLAY_BG_COLOR);
        createOutline();
        //createTicks();
        createNeedle();
        createValue();
        createEraser();
        gaugeOutline.pushSprite(DISPLAY_CENTER_X - GAUGE_RADIUS, 0);
        //gaugeTicks.pushSprite(DISPLAY_CENTER_X - GAUGE_RADIUS, 0, TFT_TRANSPARENT);
        plotNeedle(GAUGE_START_ANGLE);
        plotValue(0.0);

        if (!stats.createSprite(40, 120)) {
            Serial.println("Could not create stats");
        }
        Serial.println("Created stats, exiting initialization");

        // Initialize sweep
        sweepState = SWEEP_UP;
        sweepStartTime = millis();
        sweepValue = minValue;
        currentAngle = GAUGE_START_ANGLE;
        oldAngle = GAUGE_START_ANGLE;
    }

    void setReading(double reading) {
        targetValue = constrain(reading, minValue, maxValue);
    }

    void setCommandIndex(int newCommandIndex) {
        if (newCommandIndex < 0 || newCommandIndex >= commands->getCommandCount() || newCommandIndex == commandIndex) {
            return;
        }

        commandIndex = newCommandIndex;
        valueLabel = commands->getCommandLabel(commandIndex);
        valueUnits = commands->getCommandUnits(commandIndex);
        minValue = commands->getCommandMin(commandIndex);
        maxValue = commands->getCommandMax(commandIndex);
        valueDecimals = commands->getCommandDecimals(commandIndex);
        targetValue = minValue;
        resetSweep();
    }

    int getCommandIndex() const {
        return commandIndex;
    }

    void setThemeColors(uint16_t label, uint16_t value, uint16_t outline) override {
        labelColor = label;
        valueColor = value;
        outlineColor = outline;
    }

    uint32_t getCurrentLabelColor() override { return labelColor; }
    uint32_t getCurrentOutlineColor() override { return outlineColor; }
    uint32_t getCurrentValueColor() override { return valueColor; }

    void render(double) override {
        const unsigned long SWEEP_UP_DURATION = 1500; // 1 second up
        const unsigned long SWEEP_DOWN_DURATION = 1500; // 1 second down
        unsigned long currentTime = millis();

        if (sweepState == SWEEP_UP) {
            double progress = (double)(currentTime - sweepStartTime) / SWEEP_UP_DURATION;
            if (progress >= 1.0) {
                sweepValue = maxValue;
                currentAngle = GAUGE_END_ANGLE;
                sweepState = SWEEP_DOWN;
                sweepStartTime = currentTime;
            } else {
                sweepValue = minValue + (maxValue - minValue) * progress;
                currentAngle = GAUGE_START_ANGLE + 240 * progress;
            }
            plotNeedle(currentAngle);
            plotValue(sweepValue);
        } else if (sweepState == SWEEP_DOWN) {
            double target = targetValue > 0.0 ? targetValue : 0.0;
            double progress = (double)(currentTime - sweepStartTime) / SWEEP_DOWN_DURATION;
            if (progress >= 1.0) {
                sweepValue = target;
                currentAngle = calculateAngle(target);
                sweepState = SWEEP_COMPLETE;
            } else {
                sweepValue = maxValue - (maxValue - target) * progress;
                currentAngle = GAUGE_END_ANGLE - (GAUGE_END_ANGLE - calculateAngle(target)) * progress;
            }
            plotNeedle(currentAngle);
            plotValue(sweepValue);
        } else {
            double targetAngle = calculateAngle(targetValue);
            currentAngle += SMOOTHING_FACTOR * (targetAngle - currentAngle); // Smoothing factor in config
            plotNeedle(currentAngle);
            plotValue(targetValue);
        }
    }

    void reset() {
        display->fillScreen(DISPLAY_BG_COLOR);
        gaugeOutline.deleteSprite();
        gaugeValue.deleteSprite();
        gaugeNeedle.deleteSprite();
        gaugeEraser.deleteSprite();
        gaugeTicks.deleteSprite();
        initialize();
    }

    void displayStats(float fps, double frameAvg, double queryAvg) {
        stats.fillSprite(TFT_BLACK);
        stats.setTextFont(1);
        stats.setTextSize(1);
        stats.setTextColor(TFT_WHITE);
        stats.setRotation(0);
        stats.setCursor(0, 0);
        stats.println("FPS");
        stats.println(String(fps));
        stats.println("FrmAvg");
        stats.println(String(frameAvg));
        stats.println("QryAvg");
        stats.println(String(queryAvg));
        stats.pushSprite(0, 0);
        Serial.println("Pushed stats");
    }

    GaugeType getType() const override {
        return NEEDLE_GAUGE;
    }

private:
    Commands* commands;
    TFT_eSprite gaugeOutline, gaugeNeedle, gaugeValue, gaugeEraser, gaugeTicks, stats;
    double targetValue, currentAngle, oldAngle;
    double minValue, maxValue;
    String valueLabel, valueUnits;
    uint16_t labelColor, outlineColor, valueColor;
    int commandIndex;
    uint8_t valueDecimals;

    // Sweep state
    enum SweepState { SWEEP_UP, SWEEP_DOWN, SWEEP_COMPLETE };
    SweepState sweepState;
    unsigned long sweepStartTime;
    double sweepValue;

    void resetSweep() {
        sweepState = SWEEP_UP;
        sweepStartTime = millis();
        sweepValue = minValue;
        currentAngle = GAUGE_START_ANGLE;
        oldAngle = GAUGE_START_ANGLE;
    }

    void drawArcCaps(TFT_eSprite& target) {
        // Connect the centers of the inner and outer outline strokes at the
        // exact endpoints used by TFT_eSPI's clockwise-from-6-o'clock arcs.
        const int centerX = GAUGE_RADIUS;
        const int centerY = GAUGE_RADIUS + GAUGE_MARGIN_TOP;
        const float outerOutlineRadius = GAUGE_RADIUS - (GAUGE_LINE_WIDTH / 2.0f);
        const float innerOutlineRadius = GAUGE_RADIUS - GAUGE_ARC_WIDTH - (1.5f * GAUGE_LINE_WIDTH);
        const uint16_t capAngles[2] = {GAUGE_START_ANGLE, GAUGE_END_ANGLE};

        for (int i = 0; i < 2; i++) {
            const float angleRad = capAngles[i] * PI / 180.0f;
            const float radialX = -sinf(angleRad);
            const float radialY = cosf(angleRad);
            const float innerX = centerX + (innerOutlineRadius * radialX);
            const float innerY = centerY + (innerOutlineRadius * radialY);
            const float outerX = centerX + (outerOutlineRadius * radialX);
            const float outerY = centerY + (outerOutlineRadius * radialY);
            target.drawWideLine(innerX, innerY, outerX, outerY, GAUGE_LINE_WIDTH, outlineColor);
        }
    }

    void createOutline() {
        if (!gaugeOutline.createSprite(GAUGE_WIDTH, GAUGE_HEIGHT)) {
            Serial.println("Failed to create gauge outline");
            return;
        }
        gaugeOutline.fillSprite(GAUGE_BG_COLOR);
        gaugeOutline.drawSmoothArc(GAUGE_RADIUS, GAUGE_RADIUS + GAUGE_MARGIN_TOP, GAUGE_RADIUS, GAUGE_RADIUS - GAUGE_LINE_WIDTH, GAUGE_START_ANGLE, GAUGE_END_ANGLE, outlineColor, GAUGE_BG_COLOR, true);
        gaugeOutline.drawSmoothArc(GAUGE_RADIUS, GAUGE_RADIUS + GAUGE_MARGIN_TOP, GAUGE_RADIUS - GAUGE_LINE_WIDTH - GAUGE_ARC_WIDTH, GAUGE_RADIUS - (GAUGE_LINE_WIDTH * 2) - GAUGE_ARC_WIDTH, GAUGE_START_ANGLE, GAUGE_END_ANGLE, outlineColor, GAUGE_BG_COLOR, true);
        drawArcCaps(gaugeOutline);

        gaugeOutline.setFreeFont(FONT_BOLD_14);
        gaugeOutline.setTextColor(labelColor);
        int textWidth = gaugeOutline.textWidth(valueLabel);
        int x = (GAUGE_WIDTH - textWidth) / 2;
        gaugeOutline.drawString(valueLabel, x, GAUGE_RADIUS + GAUGE_MARGIN_TOP - 20);
        gaugeOutline.unloadFont();

        gaugeOutline.setFreeFont(FONT_NORMAL_8);
        textWidth = gaugeOutline.textWidth(valueUnits);
        x = (GAUGE_WIDTH - textWidth) / 2;
        gaugeOutline.drawString(valueUnits, x, GAUGE_RADIUS + GAUGE_MARGIN_TOP + 20);
        gaugeOutline.unloadFont();
    }

    void createTicks() {
        // Create gaugeTicks sprite
        gaugeTicks.createSprite(GAUGE_WIDTH, GAUGE_HEIGHT);
        gaugeTicks.fillSprite(TFT_TRANSPARENT);

        // Add major and minor tick marks
        int centerX = GAUGE_RADIUS;
        int centerY = GAUGE_RADIUS + GAUGE_MARGIN_TOP;

        // Draw major ticks
        for (double angleDeg = 150.0; angleDeg <= 390; angleDeg += MAJOR_TICK_SPACING) {
            double angleRad = angleDeg * PI / 180.0;
            int outerX = centerX + GAUGE_RADIUS * cos(angleRad);
            int outerY = centerY + GAUGE_RADIUS * sin(angleRad);
            int innerX = centerX + (GAUGE_RADIUS - MAJOR_TICK_LENGTH) * cos(angleRad);
            int innerY = centerY + (GAUGE_RADIUS - MAJOR_TICK_LENGTH) * sin(angleRad);
            gaugeOutline.drawWideLine(outerX, outerY, innerX, innerY, MAJOR_TICK_THICKNESS, outlineColor);
        }

        // Draw minor ticks (skip angles where major ticks are drawn)
        for (double angleDeg = 150; angleDeg <= 390; angleDeg += MINOR_TICK_SPACING) {
            if (fmod(angleDeg + 120.0, MAJOR_TICK_SPACING) != 0) { // Avoid overlap with major ticks
                double angleRad = angleDeg * PI / 180.0;
                int outerX = centerX + GAUGE_RADIUS * cos(angleRad);
                int outerY = centerY + GAUGE_RADIUS * sin(angleRad);
                int innerX = centerX + (GAUGE_RADIUS - MINOR_TICK_LENGTH) * cos(angleRad);
                int innerY = centerY + (GAUGE_RADIUS - MINOR_TICK_LENGTH) * sin(angleRad);
                gaugeOutline.drawWideLine(outerX, outerY, innerX, innerY, MINOR_TICK_THICKNESS, outlineColor);
            }
        }
    }

    void createNeedle() {
        if (!gaugeNeedle.createSprite(GAUGE_WIDTH, GAUGE_HEIGHT)) {
            Serial.println("Failed to create needle sprite");
            return;
        }
        gaugeNeedle.fillSprite(TFT_TRANSPARENT);
    }

    void createValue() {
        if (!gaugeValue.createSprite(VALUE_WIDTH, VALUE_HEIGHT)) {
            Serial.println("Failed to create value sprite");
            return;
        }
        gaugeValue.fillSprite(VALUE_BG_COLOR);
        gaugeValue.setFreeFont(FONT_BOLD_18);
        gaugeValue.setTextColor(valueColor);
    }

    void createEraser() {
        if (!gaugeEraser.createSprite(NEEDLE_SPRITE_WIDTH + 3, NEEDLE_LENGTH)) {
            Serial.println("Failed to create needle sprite");
            return;
        }
        gaugeEraser.fillSprite(GAUGE_BG_COLOR);
        int pivX = NEEDLE_SPRITE_WIDTH / 2;
        int pivY = NEEDLE_RADIUS;
        gaugeEraser.setPivot(pivX, pivY);
    }

    void plotNeedle(double newAngle) {
        // Only draw when angle change is > 1deg
        if (abs(newAngle - oldAngle) < 1.0) {
            return;
        }
        
        Serial.println("NewAngle: " + String(newAngle));
        Serial.println("OldAngle: " + String(oldAngle));
        if ((newAngle - oldAngle) < 0) {
            // Value is decreasing, draw an 'eraser'
            Serial.println("Decreasing arc");
            gaugeNeedle.drawSmoothArc(GAUGE_RADIUS, GAUGE_RADIUS + GAUGE_MARGIN_TOP, GAUGE_RADIUS - GAUGE_LINE_WIDTH - 2, GAUGE_RADIUS - GAUGE_LINE_WIDTH - GAUGE_ARC_WIDTH + 2, newAngle, GAUGE_END_ANGLE, GAUGE_BG_COLOR, GAUGE_BG_COLOR, false);
        }
        else {
            // Draw Arc
            gaugeNeedle.drawSmoothArc(GAUGE_RADIUS, GAUGE_RADIUS + GAUGE_MARGIN_TOP, GAUGE_RADIUS - GAUGE_LINE_WIDTH - 2, GAUGE_RADIUS - GAUGE_LINE_WIDTH - GAUGE_ARC_WIDTH + 2, GAUGE_START_ANGLE, newAngle, valueColor, GAUGE_BG_COLOR, false);
        }
        
        // Restore the caps after the opaque eraser/needle arc crosses them.
        drawArcCaps(gaugeNeedle);

        // Push arc then ticks over top
        gaugeNeedle.pushSprite(DISPLAY_CENTER_X - GAUGE_RADIUS, 0, TFT_TRANSPARENT);
        //gaugeTicks.pushSprite(DISPLAY_CENTER_X - GAUGE_RADIUS, 0, TFT_TRANSPARENT);

        oldAngle = newAngle;
    }

    void plotValue(double val) {
        gaugeValue.fillSprite(VALUE_BG_COLOR);
        gaugeValue.setFreeFont(FONT_BOLD_18);
        gaugeValue.setTextColor(valueColor, DISPLAY_BG_COLOR);

        if (valueDecimals == 0) {
            int intVal = (int)round(val);
            int textWidth = gaugeValue.textWidth(String(intVal));
            int x = (VALUE_WIDTH - textWidth) / 2;
            gaugeValue.drawNumber(intVal, x, 0);
        } else {
            int textWidth = gaugeValue.textWidth(String(val, static_cast<unsigned int>(valueDecimals)));
            int x = (VALUE_WIDTH - textWidth) / 2;
            gaugeValue.drawFloat(val, valueDecimals, x, 0);
        }
        gaugeValue.pushSprite(VALUE_X, VALUE_Y + GAUGE_MARGIN_TOP);
        gaugeValue.unloadFont();
    }

    double calculateAngle(double value) {
        const double range = maxValue - minValue;
        const double normalized = range > 0.0 ? (value - minValue) / range : 0.0;
        double angle = (double)GAUGE_START_ANGLE + (normalized * 240.0);
        return constrain(angle, GAUGE_START_ANGLE, GAUGE_END_ANGLE);
    }
};

#endif
