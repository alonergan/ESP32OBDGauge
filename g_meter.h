#ifndef G_METER_H
#define G_METER_H

#include "gauge.h"
#include "config.h"
#include "gmeter_calibration.h"

class GMeter : public Gauge {
public:
    GMeter(TFT_eSPI* display) :
        Gauge(display),
        mpu(),
        combined(display),
        history(display),
        maxX(display),
        maxY(display),
        minX(display),
        minY(display),
        minValueX(0.0),
        minValueY(0.0),
        maxValueX(0.0),
        maxValueY(0.0),
        oldX(0),
        oldY(0),
        filteredX(0.0),
        filteredY(0.0),
        hasFilter(false),
        calibrationDirty(false) {}

    void initialize() override {
        display->fillScreen(DISPLAY_BG_COLOR);

        int gaugeCenterX = DISPLAY_CENTER_X;
        int gaugeCenterY = DISPLAY_CENTER_Y;
        int outlineX = gaugeCenterX - GMETER_RADIUS;
        int outlineY = gaugeCenterY - GMETER_RADIUS;

        combined.setColorDepth(8);
        combined.createSprite(GMETER_WIDTH + 2, GMETER_HEIGHT + 2);
        combined.fillSprite(TFT_TRANSPARENT);

        history.setColorDepth(8);
        history.createSprite(GMETER_WIDTH + 2, GMETER_HEIGHT + 2);
        history.fillSprite(TFT_TRANSPARENT);

        drawOutline();
        combined.fillCircle(GMETER_RADIUS, GMETER_RADIUS, GMETER_POINT_RADIUS, GMETER_POINT_COLOR);

        createTextSprite(minX, "0.00");
        createTextSprite(maxX, "0.00");
        createTextSprite(minY, "0.00");
        createTextSprite(maxY, "0.00");

        combined.pushSprite(outlineX, outlineY, TFT_TRANSPARENT);
        pushCenteredSprite(minX, outlineX + GMETER_WIDTH + GMETER_TEXT_OFFSET_X, gaugeCenterY);
        pushCenteredSprite(maxX, outlineX - GMETER_TEXT_OFFSET_X, gaugeCenterY);
        pushCenteredSprite(minY, gaugeCenterX, outlineY - GMETER_TEXT_OFFSET_Y);
        pushCenteredSprite(maxY, gaugeCenterX, outlineY + GMETER_HEIGHT + GMETER_TEXT_OFFSET_Y);

        mpu.begin();
        mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
        mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);

        oldX = gaugeCenterX - GMETER_POINT_RADIUS;
        oldY = gaugeCenterY - GMETER_POINT_RADIUS;
    }

    void setAccelData(sensors_event_t* accel, sensors_event_t* gyro, sensors_event_t* temp) {
        latestAccel = *accel;
        latestGyro = *gyro;
        latestTemp = *temp;
        calibration.addSample(latestAccel);
        if (!calibration.isCollecting() && calibration.isCalibrated() && calibrationInProgress) {
            calibrationInProgress = false;
            calibrationDirty = true;
        }
    }

    void render(double) override {
        if (calibration.isCollecting()) {
            display->setTextColor(TFT_YELLOW, DISPLAY_BG_COLOR);
            display->setTextSize(1);
            display->setCursor(5, 5);
            int pct = (calibration.getSampleCount() * 100) / calibration.getRequiredSamples();
            display->printf("Calibrating GMeter... %d%%   ", pct);
            return;
        }

        Vec3 normalized = calibration.normalize(latestAccel);
        double gForceX = -normalized.y;
        double gForceY = -normalized.x;

        if (!isfinite(gForceX) || !isfinite(gForceY)) {
            return;
        }

        // Reject impossible spikes and smooth incoming data to avoid outlier history points.
        if (fabs(gForceX) > 3.0 || fabs(gForceY) > 3.0) {
            return;
        }

        if (!hasFilter) {
            filteredX = gForceX;
            filteredY = gForceY;
            hasFilter = true;
        }

        double dx = gForceX - filteredX;
        double dy = gForceY - filteredY;
        const double spikeLimit = 0.45;
        if (fabs(dx) > spikeLimit || fabs(dy) > spikeLimit) {
            return;
        }

        const double alpha = 0.25;
        filteredX = filteredX + alpha * dx;
        filteredY = filteredY + alpha * dy;

        if (fabs(filteredX) < 0.02) filteredX = 0.0;
        if (fabs(filteredY) < 0.02) filteredY = 0.0;

        int gaugeCenterX = DISPLAY_CENTER_X;
        int gaugeCenterY = DISPLAY_CENTER_Y;
        int posX = gaugeCenterX - (GMETER_RADIUS * filteredX / 2);
        int posY = gaugeCenterY - (GMETER_RADIUS * filteredY / 2);

        if (abs(posX - oldX) > 1 || abs(posY - oldY) > 1) {
            drawOutline();
            combined.fillCircle(posX - (gaugeCenterX - GMETER_RADIUS), posY - (gaugeCenterY - GMETER_RADIUS), GMETER_POINT_RADIUS, GMETER_POINT_COLOR);
            history.fillCircle(posX - (gaugeCenterX - GMETER_RADIUS), posY - (gaugeCenterY - GMETER_RADIUS), GMETER_POINT_RADIUS, GMETER_HISTORY_COLOR);
            history.pushSprite(gaugeCenterX - GMETER_RADIUS, gaugeCenterY - GMETER_RADIUS, TFT_TRANSPARENT);
            combined.pushSprite(gaugeCenterX - GMETER_RADIUS, gaugeCenterY - GMETER_RADIUS, TFT_TRANSPARENT);
            oldX = posX;
            oldY = posY;
        }

        updateMinMaxValues(filteredX, filteredY);
    }

    void beginManualCalibration() {
        calibration.startCalibration();
        calibrationInProgress = true;
        calibrationDirty = false;
        hasFilter = false;
    }

    bool consumeCalibration(float outRotation[3][3], Vec3& outBias) {
        if (!calibrationDirty) {
            return false;
        }
        calibration.getCalibration(outRotation, outBias);
        calibrationDirty = false;
        return true;
    }

    void setStoredCalibration(const float inRotation[3][3], const Vec3& inBias) {
        calibration.setCalibration(inRotation, inBias);
        calibrationInProgress = false;
        calibrationDirty = false;
    }

    void reset() {
        minValueX = 0.0;
        minValueY = 0.0;
        maxValueX = 0.0;
        maxValueY = 0.0;
        hasFilter = false;
        history.fillSprite(TFT_TRANSPARENT);
        updateTextSprite(maxX, "0.00");
        updateTextSprite(minX, "0.00");
        updateTextSprite(maxY, "0.00");
        updateTextSprite(minY, "0.00");
    }

    void displayStats(float fps, double frameAvg, double queryAvg) override {
        display->setTextColor(TFT_WHITE, DISPLAY_BG_COLOR);
        display->setTextSize(1);
        display->setCursor(0, 0);
        display->printf("FPS: %.1f\nFrame: %.1f ms\nQuery: %.1f ms", fps, frameAvg, queryAvg);
    }

    GaugeType getType() const override { return G_METER; }
    uint32_t getCurrentNeedleColor() { return 0; }
    uint32_t getCurrentOutlineColor() { return 0; }
    uint32_t getCurrentValueColor() { return 0; }

private:
    Adafruit_MPU6050 mpu;
    TFT_eSprite combined, history, maxX, maxY, minX, minY;
    sensors_event_t latestAccel, latestGyro, latestTemp;
    double minValueX, maxValueX, minValueY, maxValueY;
    int oldX, oldY;
    double filteredX, filteredY;
    bool hasFilter;
    bool calibrationInProgress = false;
    bool calibrationDirty;
    GMeterCalibration calibration;

    void drawOutline() {
        combined.fillSprite(TFT_TRANSPARENT);
        int lineSize = GMETER_LINE_SIZE;
        for (int i = 3; i > 0; i--) {
            int lineRadius = i * (GMETER_RADIUS / 4);
            combined.drawLine(GMETER_RADIUS - lineRadius, GMETER_RADIUS - lineSize, GMETER_RADIUS - lineRadius, GMETER_RADIUS + lineSize, GMETER_OUTLINE_COLOR);
            combined.drawLine(GMETER_RADIUS + lineRadius, GMETER_RADIUS - lineSize, GMETER_RADIUS + lineRadius, GMETER_RADIUS + lineSize, GMETER_OUTLINE_COLOR);
            combined.drawLine(GMETER_RADIUS - lineSize, GMETER_RADIUS - lineRadius, GMETER_RADIUS + lineSize, GMETER_RADIUS - lineRadius, GMETER_OUTLINE_COLOR);
            combined.drawLine(GMETER_RADIUS - lineSize, GMETER_RADIUS + lineRadius, GMETER_RADIUS + lineSize, GMETER_RADIUS + lineRadius, GMETER_OUTLINE_COLOR);
        }
        combined.drawSmoothCircle(GMETER_RADIUS, GMETER_RADIUS, GMETER_RADIUS, GMETER_OUTLINE_COLOR, TFT_TRANSPARENT);
    }

    void createTextSprite(TFT_eSprite& sprite, const char* text) {
        sprite.setColorDepth(8);
        sprite.setTextFont(GMETER_TEXT_FONT);
        sprite.setTextSize(GMETER_TEXT_SIZE);
        sprite.setTextColor(GMETER_TEXT_COLOR);
        int textWidth = sprite.textWidth("0.00");
        int textHeight = sprite.fontHeight();
        sprite.createSprite(textWidth + 10, textHeight + 10);
        sprite.fillSprite(DISPLAY_BG_COLOR);
        sprite.setCursor((sprite.width() - textWidth) / 2, (sprite.height() - textHeight) / 2);
        sprite.println(text);
    }

    void pushCenteredSprite(TFT_eSprite& sprite, int x, int y) {
        sprite.pushSprite(x - sprite.width() / 2, y - sprite.height() / 2);
    }

    void updateMinMaxValues(double gForceX, double gForceY) {
        int outlineX = DISPLAY_CENTER_X - GMETER_RADIUS;
        int outlineY = DISPLAY_CENTER_Y - GMETER_RADIUS;

        if (gForceX > maxValueX) {
            maxValueX = gForceX;
            updateTextSprite(maxX, String(maxValueX, 2));
            pushCenteredSprite(maxX, outlineX - GMETER_TEXT_OFFSET_X, DISPLAY_CENTER_Y);
        }
        if (gForceX < minValueX) {
            minValueX = gForceX;
            updateTextSprite(minX, String(abs(minValueX), 2));
            pushCenteredSprite(minX, outlineX + GMETER_WIDTH + GMETER_TEXT_OFFSET_X, DISPLAY_CENTER_Y);
        }
        if (gForceY > maxValueY) {
            maxValueY = gForceY;
            updateTextSprite(maxY, String(maxValueY, 2));
            pushCenteredSprite(maxY, DISPLAY_CENTER_X, outlineY - GMETER_TEXT_OFFSET_Y);
        }
        if (gForceY < minValueY) {
            minValueY = gForceY;
            updateTextSprite(minY, String(abs(minValueY), 2));
            pushCenteredSprite(minY, DISPLAY_CENTER_X, outlineY + GMETER_HEIGHT + GMETER_TEXT_OFFSET_Y);
        }
    }

    void updateTextSprite(TFT_eSprite& sprite, String text) {
        sprite.fillSprite(DISPLAY_BG_COLOR);
        int textWidth = sprite.textWidth(text);
        int textHeight = sprite.fontHeight();
        sprite.setCursor((sprite.width() - textWidth) / 2, (sprite.height() - textHeight) / 2);
        sprite.println(text);
    }
};

#endif
