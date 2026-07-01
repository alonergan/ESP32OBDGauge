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
        speedAvailable(false),
        displayedSpeedAvailable(false),
        displayedSpeed(-1),
        previousSpeed(0.0),
        runTimeSeconds(0.0),
        stationaryStartMs(0),
        startTimeUs(0),
        previousTimeUs(0),
        stage(WAIT_STATIONARY) {
        (void)outlineColor; (void)labelColor; (void)valueColor;
    }

    void initialize() override {
        display->fillScreen(DISPLAY_BG_COLOR);

        speed.setColorDepth(8);
        speed.setFreeFont(FONT_BOLD_30);
        speed.setTextColor(TFT_RED, DISPLAY_BG_COLOR);
        speed.createSprite(SPEED_VALUE_WIDTH, speed.fontHeight() + VALUE_VERTICAL_PADDING);
        speed.unloadFont();
        displayedSpeed = -1;

        speedLabel.setColorDepth(1);
        speedLabel.setFreeFont(FONT_BOLD_16);
        speedLabel.setTextColor(TFT_WHITE);
        speedLabel.createSprite(speedLabel.textWidth(AMETER_SPEED_LABEL), speedLabel.fontHeight());
        speedLabel.drawString(AMETER_SPEED_LABEL, 0, 0);
        speedLabel.unloadFont();

        time.setColorDepth(8);
        time.setFreeFont(FONT_BOLD_22);
        time.setTextColor(AMETER_TIME_COLOR, DISPLAY_BG_COLOR);
        time.createSprite(TIME_VALUE_WIDTH, time.fontHeight() + VALUE_VERTICAL_PADDING);
        time.unloadFont();

        timeLabel.setColorDepth(1);
        timeLabel.setFreeFont(FONT_BOLD_12);
        timeLabel.setTextColor(TFT_WHITE);
        timeLabel.createSprite(timeLabel.textWidth(AMETER_TIME_LABEL), timeLabel.fontHeight());
        timeLabel.drawString(AMETER_TIME_LABEL, 0, 0);
        timeLabel.unloadFont();

        message.setColorDepth(8);
        message.setTextColor(TFT_YELLOW, DISPLAY_BG_COLOR);
        message.createSprite(DISPLAY_WIDTH, 20);

        pushCenteredAt(speedLabel, SPEED_LABEL_CENTER_Y);
        pushCenteredAt(timeLabel, TIME_LABEL_CENTER_Y);

        drawSpeed((int)round(latestSpeed));
        drawTime(runTimeSeconds, stage == FINISHED ? TFT_GREEN : AMETER_TIME_COLOR);
        drawMessageForStage();
    }

    void setSpeed(double speedVal, bool available = true) {
        speedAvailable = available && isfinite(speedVal) && speedVal >= 0.0;
        if (!speedAvailable) {
            speedVal = 0;
        }

        latestSpeed = speedVal;
    }

    void render(double) override {
        unsigned long nowMs = millis();
        unsigned long long nowUs = micros();

        drawSpeed((int)round(latestSpeed));

        if (speedAvailable != displayedSpeedAvailable) {
            displayedSpeedAvailable = speedAvailable;
            drawMessageForStage();
        }
        if (!speedAvailable) {
            return;
        }

        switch (stage) {
            case WAIT_STATIONARY:
                if (latestSpeed <= 1.0) {
                    if (stationaryStartMs == 0) {
                        stationaryStartMs = nowMs;
                    }
                    if (nowMs - stationaryStartMs >= 3000) {
                        stage = READY;
                        runTimeSeconds = 0.0;
                        drawTime(runTimeSeconds, AMETER_TIME_COLOR);
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
                    drawTime(runTimeSeconds, TFT_GREEN);
                    drawMessageForStage();
                } else {
                    drawTime(runTimeSeconds, AMETER_TIME_COLOR);
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
        (void)label; (void)value; (void)outline;
    }
    uint32_t getCurrentLabelColor() override { return TFT_WHITE; }
    uint32_t getCurrentOutlineColor() override { return TFT_WHITE; }
    uint32_t getCurrentValueColor() override { return TFT_RED; }

private:
    enum MeterStage { WAIT_STATIONARY, READY, RUNNING, FINISHED };

    static const int SPEED_VALUE_WIDTH = 140;
    static const int TIME_VALUE_WIDTH = 200;
    static const int VALUE_VERTICAL_PADDING = 10;
    static const int SPEED_LABEL_CENTER_Y = 28;
    static const int SPEED_VALUE_CENTER_Y = 80;
    static const int TIME_LABEL_CENTER_Y = 143;
    static const int TIME_VALUE_CENTER_Y = 185;

    TFT_eSprite time, speed, timeLabel, speedLabel, message;
    double latestSpeed;
    bool speedAvailable;
    bool displayedSpeedAvailable;
    int displayedSpeed;
    double previousSpeed;
    double runTimeSeconds;
    unsigned long stationaryStartMs;
    unsigned long long startTimeUs;
    unsigned long long previousTimeUs;
    MeterStage stage;

    void pushCenteredAt(TFT_eSprite& sprite, int centerY) {
        sprite.pushSprite((DISPLAY_WIDTH - sprite.width()) / 2, centerY - sprite.height() / 2);
    }

    void drawSpeed(int speedInt) {
        if (speedInt == displayedSpeed) {
            return;
        }

        String text = String(speedInt);
        speed.fillSprite(DISPLAY_BG_COLOR);
        speed.setFreeFont(FONT_BOLD_30);
        speed.setTextDatum(MC_DATUM);
        speed.drawString(text, speed.width() / 2, speed.height() / 2);
        speed.setTextDatum(TL_DATUM);
        speed.unloadFont();
        pushCenteredAt(speed, SPEED_VALUE_CENTER_Y);
        displayedSpeed = speedInt;
    }

    void drawTime(double seconds, uint16_t color) {
        char timeStr[16];
        snprintf(timeStr, sizeof(timeStr), "%0.3f", seconds);
        String text(timeStr);

        time.fillSprite(DISPLAY_BG_COLOR);
        time.setFreeFont(FONT_BOLD_22);
        time.setTextColor(color, DISPLAY_BG_COLOR);
        time.setTextDatum(MC_DATUM);
        time.drawString(timeStr, time.width() / 2, time.height() / 2);
        time.setTextDatum(TL_DATUM);
        time.unloadFont();
        pushCenteredAt(time, TIME_VALUE_CENTER_Y);
    }

    void drawMessageForStage() {
        const char* text = "";
        uint16_t color = TFT_YELLOW;

        if (!speedAvailable) {
            text = "Unable to read vehicle speed";
        } else if (stage == WAIT_STATIONARY) {
            text = "Keep vehicle still for 3s to arm";
        } else if (stage == READY) {
            text = "Ready - accelerate to start";
            color = TFT_CYAN;
        } else if (stage == RUNNING) {
            text = "Measuring 0-60...";
            color = TFT_ORANGE;
        } else if (stage == FINISHED) {
            text = "Done - tap reset button";
            color = TFT_GREEN;
        }

        message.fillSprite(DISPLAY_BG_COLOR);
        message.setFreeFont(FONT_NORMAL_8);
        message.setTextColor(color, DISPLAY_BG_COLOR);
        message.setTextDatum(MC_DATUM);
        message.drawString(text, DISPLAY_WIDTH / 2, 8);
        message.setTextDatum(TL_DATUM);
        message.unloadFont();
        message.pushSprite(0, DISPLAY_HEIGHT - message.height());
    }
};

#endif
