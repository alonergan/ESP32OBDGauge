#ifndef ACCEL_METER_H
#define ACCEL_METER_H

#include "gauge.h"

class AccelerationMeter : public Gauge {
public:
    AccelerationMeter(TFT_eSPI* display, uint16_t outlineColor, uint16_t labelColor, uint16_t valueColor) :
        Gauge(display),
        time(display),
        timeLabel(display),
        speed(display),
        speedLabel(display),
        message(display),
        latestSpeed(0.0),
        displayedSpeed(-1),
        previousSpeed(0.0),
        runTimeSeconds(0.0),
        stationaryStartMs(0),
        startTimeUs(0),
        previousTimeUs(0),
        stage(WAIT_STATIONARY),
        outlineColor(outlineColor),
        labelColor(labelColor),
        valueColor(valueColor) {}

    void initialize() override {
        display->fillScreen(DISPLAY_BG_COLOR);
        display->drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, outlineColor);

        speed.setColorDepth(8);
        speed.setTextSize(7);
        speed.setTextFont(1);
        speed.setTextColor(valueColor, DISPLAY_BG_COLOR);
        speed.createSprite(speed.textWidth("188"), speed.fontHeight());

        speedLabel.setColorDepth(8);
        speedLabel.setTextFont(1);
        speedLabel.setTextSize(3);
        speedLabel.setTextColor(labelColor);
        speedLabel.createSprite(speedLabel.textWidth(AMETER_SPEED_LABEL), speedLabel.fontHeight());
        speedLabel.setCursor(0, 0);
        speedLabel.println(AMETER_SPEED_LABEL);

        time.setColorDepth(8);
        time.setTextSize(5);
        time.setTextFont(1);
        time.setTextColor(valueColor, DISPLAY_BG_COLOR);
        time.createSprite(time.textWidth("00.000"), time.fontHeight());

        timeLabel.setColorDepth(8);
        timeLabel.setTextFont(1);
        timeLabel.setTextSize(3);
        timeLabel.setTextColor(labelColor);
        timeLabel.createSprite(timeLabel.textWidth(AMETER_TIME_LABEL), timeLabel.fontHeight());
        timeLabel.setCursor(0, 0);
        timeLabel.println(AMETER_TIME_LABEL);

        message.setColorDepth(8);
        message.setTextFont(2);
        message.setTextSize(1);
        message.setTextColor(labelColor, DISPLAY_BG_COLOR);
        message.createSprite(DISPLAY_WIDTH, 20);

        speedLabel.pushSprite((DISPLAY_WIDTH - speedLabel.width()) / 2, 60 - speedLabel.height() - AMETER_V_PADDING);
        timeLabel.pushSprite((DISPLAY_WIDTH - timeLabel.width()) / 2, 175 - timeLabel.height() - AMETER_V_PADDING);

        drawSpeed((int)round(latestSpeed));
        drawTime(runTimeSeconds, valueColor);
        drawMessageForStage();
    }

    void setSpeed(double speedVal) {
        latestSpeed = speedVal;
    }

    void render(double) override {
        unsigned long nowMs = millis();
        unsigned long long nowUs = micros();

        drawSpeed((int)round(latestSpeed));

        switch (stage) {
            case WAIT_STATIONARY:
                if (latestSpeed <= 1.0) {
                    if (stationaryStartMs == 0) {
                        stationaryStartMs = nowMs;
                    }
                    if (nowMs - stationaryStartMs >= 3000) {
                        stage = READY;
                        runTimeSeconds = 0.0;
                        drawTime(runTimeSeconds, valueColor);
                        drawMessageForStage();
                    }
                } else {
                    stationaryStartMs = 0;
                }
                break;

            case READY:
                if (latestSpeed > 1.5) {
                    stage = RUNNING;
                    startTimeUs = nowUs;
                    previousTimeUs = nowUs;
                    previousSpeed = latestSpeed;
                    drawMessageForStage();
                }
                break;

            case RUNNING:
                runTimeSeconds = (double)(nowUs - startTimeUs) / 1000000.0;

                if (latestSpeed >= 60.0) {
                    if (latestSpeed > previousSpeed && previousSpeed < 60.0) {
                        double fraction = (60.0 - previousSpeed) / (latestSpeed - previousSpeed);
                        unsigned long long crossedUs = previousTimeUs + (unsigned long long)(fraction * (double)(nowUs - previousTimeUs));
                        runTimeSeconds = (double)(crossedUs - startTimeUs) / 1000000.0;
                    }
                    stage = FINISHED;
                    drawTime(runTimeSeconds, valueColor);
                    drawMessageForStage();
                } else {
                    drawTime(runTimeSeconds, valueColor);
                }

                previousSpeed = latestSpeed;
                previousTimeUs = nowUs;
                break;

            case FINISHED:
                // Latch final run time until reset() is called by user.
                break;
        }
    }

    void displayStats(float fps, double frameAvg, double queryAvg) override {
        display->setTextColor(TFT_WHITE, DISPLAY_BG_COLOR);
        display->setTextSize(1);
        display->setCursor(0, 0);
        display->printf("FPS: %.1f\nFrame: %.1f ms\nQuery: %.1f ms", fps, frameAvg, queryAvg);
    }

    GaugeType getType() const override {
        return ACCELERATION_METER;
    }

    void reset() override {
        stage = WAIT_STATIONARY;
        stationaryStartMs = 0;
        startTimeUs = 0;
        previousTimeUs = 0;
        previousSpeed = latestSpeed;
        runTimeSeconds = 0.0;
        displayedSpeed = -1;
        initialize();
    }

    void setThemeColors(uint16_t label, uint16_t value, uint16_t outline) override {
        labelColor = label; valueColor = value; outlineColor = outline;
    }
    uint32_t getCurrentLabelColor() override { return labelColor; }
    uint32_t getCurrentOutlineColor() override { return outlineColor; }
    uint32_t getCurrentValueColor() override { return valueColor; }

private:
    enum MeterStage { WAIT_STATIONARY, READY, RUNNING, FINISHED };

    TFT_eSprite time, speed, timeLabel, speedLabel, message;
    double latestSpeed;
    int displayedSpeed;
    double previousSpeed;
    double runTimeSeconds;
    unsigned long stationaryStartMs;
    unsigned long long startTimeUs;
    unsigned long long previousTimeUs;
    MeterStage stage;
    uint16_t outlineColor;
    uint16_t labelColor;
    uint16_t valueColor;

    void drawSpeed(int speedInt) {
        if (speedInt == displayedSpeed) {
            return;
        }

        String text = String(speedInt);
        speed.fillSprite(DISPLAY_BG_COLOR);
        int textWidth = speed.textWidth(text);
        int textHeight = speed.fontHeight();
        speed.setCursor((speed.width() - textWidth) / 2, (speed.height() - textHeight) / 2);
        speed.println(text);
        speed.pushSprite((DISPLAY_WIDTH - speed.width()) / 2, 60);
        displayedSpeed = speedInt;
    }

    void drawTime(double seconds, uint16_t color) {
        char timeStr[16];
        snprintf(timeStr, sizeof(timeStr), "%0.3f", seconds);
        String text(timeStr);

        time.fillSprite(DISPLAY_BG_COLOR);
        time.setTextColor(color, DISPLAY_BG_COLOR);
        int textWidth = time.textWidth(text);
        int textHeight = time.fontHeight();
        time.setCursor((time.width() - textWidth) / 2, (time.height() - textHeight) / 2);
        time.println(text);
        time.pushSprite((DISPLAY_WIDTH - time.width()) / 2, 175);
    }

    void drawMessageForStage() {
        const char* text = "";
        uint16_t color = labelColor;

        if (stage == WAIT_STATIONARY) {
            text = "Keep vehicle still for 3s to arm";
        } else if (stage == READY) {
            text = "Ready - accelerate to start";
        } else if (stage == RUNNING) {
            text = "Measuring 0-60...";
        } else if (stage == FINISHED) {
            text = "Done - tap reset button";
        }

        message.fillSprite(DISPLAY_BG_COLOR);
        message.setTextColor(color, DISPLAY_BG_COLOR);
        int textWidth = message.textWidth(text);
        message.setCursor((DISPLAY_WIDTH - textWidth) / 2, 4);
        message.println(text);
        message.pushSprite(0, DISPLAY_HEIGHT - message.height());
    }
};

#endif
