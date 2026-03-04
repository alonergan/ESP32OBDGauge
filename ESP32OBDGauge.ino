#include <TFT_eSPI.h>
#include <Preferences.h>
#include "touch.h"
#include "bluetooth.h"
#include "commands.h"
#include "needle_gauge.h"
#include "dual_gauge.h"
#include "g_meter.h"
#include "acceleration_meter.h"
#include "quadrant_gauge.h"
#include "options_screen.h"
#include "config.h"
#include "screen_manager.h"

bool TESTMODE = true;

/*
    Variables for settings stored in flash memory
*/
uint32_t outlineColor = GAUGE_FG_COLOR;
uint32_t needleColor = NEEDLE_COLOR_PRIMARY;
uint32_t valueColor = VALUE_TEXT_COLOR;
int currentGauge = 0;
float mpuCalibrationMatrix[3][3]; // Persisted G-meter rotation matrix
Vec3 mpuCalibrationBias = {0.0f, 0.0f, 0.0f};
bool gmeterCalibrationStored = false;

Preferences preferences;
TFT_eSPI display = TFT_eSPI();
const int GAUGE_COUNT = 8;
const int FALLBACK_GAUGE_INDEX = 5;
Gauge* gauges[GAUGE_COUNT];
ScreenManager screenManager;
Commands commands;
TaskHandle_t dataTaskHandle;
SemaphoreHandle_t gaugeMutex;
unsigned long lastTouchTime = 0;
bool obdConnected = false;
OptionsScreen* optionsScreen = nullptr;
bool inOptionsScreen = false;

void setup() {
    Serial.begin(115200);

    readSettings();

    display.begin();
    display.setRotation(3);
    touch_init(DISPLAY_WIDTH, DISPLAY_HEIGHT, display.getRotation());

    // Show splash screen
    int r = 50;
    display.fillScreen(TFT_BLACK);
    display.drawSmoothArc(61, DISPLAY_CENTER_Y, r, r-10, 0, 360, TFT_WHITE, TFT_BLACK);
    delay(250);
    display.drawSmoothArc(127, DISPLAY_CENTER_Y, r, r-10, 0, 360, TFT_WHITE, TFT_BLACK);
    delay(250);
    display.drawSmoothArc(193, DISPLAY_CENTER_Y, r, r-10, 0, 360, TFT_WHITE, TFT_BLACK);
    delay(250);
    display.drawSmoothArc(259, DISPLAY_CENTER_Y, r, r-10, 0, 360, TFT_WHITE, TFT_BLACK);
    delay(1000);

    if (!TESTMODE) {
        obdConnected = connectToOBD();
    } else {
        obdConnected = true;
    }

    gaugeMutex = xSemaphoreCreateMutex();
    gauges[0] = new NeedleGauge(&display, 0, outlineColor, needleColor, valueColor); // RPM    (Actual)
    gauges[1] = new NeedleGauge(&display, 1, outlineColor, needleColor, valueColor); // Boost  (Approximate)
    gauges[2] = new NeedleGauge(&display, 2, outlineColor, needleColor, valueColor); // Torque (Approximate)
    gauges[3] = new NeedleGauge(&display, 3, outlineColor, needleColor, valueColor); // Horsepower (Approximate)
    gauges[4] = new DualGauge(&display, 0, outlineColor, needleColor, valueColor);
    gauges[5] = new GMeter(&display);
    gauges[6] = new AccelerationMeter(&display);
    gauges[7] = new QuadrantGauge(&display, &commands);

    if (gmeterCalibrationStored) {
        static_cast<GMeter*>(gauges[5])->setStoredCalibration(mpuCalibrationMatrix, mpuCalibrationBias);
    }

    if (!obdConnected) {
        currentGauge = FALLBACK_GAUGE_INDEX;
    }

    screenManager.initialize(gauges, GAUGE_COUNT, currentGauge);
    screenManager.getCurrentGauge()->initialize();

    xTaskCreatePinnedToCore(dataFetchingTask, "DataFetching", 10000, NULL, 1, &dataTaskHandle, 0);
}

void readSettings() {
    // Initialize preferences
    preferences.begin("OBDGAUGE", false);

    // Try to get stored settings
    outlineColor = preferences.getUInt("outlineColor", GAUGE_FG_COLOR);
    needleColor = preferences.getUInt("needleColor", NEEDLE_COLOR_PRIMARY);
    valueColor = preferences.getUInt("valueColor", VALUE_TEXT_COLOR);
    currentGauge = preferences.getInt("currentGauge", 0);

    gmeterCalibrationStored = preferences.getBool("gmCalSaved", false);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            String key = "gmr" + String(i) + String(j);
            mpuCalibrationMatrix[i][j] = preferences.getFloat(key.c_str(), (i == j) ? 1.0f : 0.0f);
        }
    }
    mpuCalibrationBias.x = preferences.getFloat("gmbx", 0.0f);
    mpuCalibrationBias.y = preferences.getFloat("gmby", 0.0f);
    mpuCalibrationBias.z = preferences.getFloat("gmbz", 0.0f);

    // Close preferences
    preferences.end();
    return;
}

void updateCurrentGaugeSettings(int newCurrentGauge) {
    // Initialize preferences
    preferences.begin("OBDGAUGE", false);

    // Set new settings 
    preferences.putInt("currentGauge", newCurrentGauge);

    // Close preferences
    preferences.end();
    return;
}

void updateCurrentGaugeColorSettings(uint32_t currNeedleColor, uint32_t currOutlineColor, uint32_t currValueColor) {
    // Initialize preferences
    preferences.begin("OBDGAUGE", false);

    // Set new settings 
    preferences.putUInt("needleColor", currNeedleColor);
    preferences.putUInt("outlineColor", currOutlineColor);
    preferences.putUInt("valueColor", currValueColor);

    // Close preferences
    preferences.end();
    return;
}


void updateGMeterCalibrationSettings(const float rotation[3][3], const Vec3& bias) {
    preferences.begin("OBDGAUGE", false);

    preferences.putBool("gmCalSaved", true);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            String key = "gmr" + String(i) + String(j);
            preferences.putFloat(key.c_str(), rotation[i][j]);
        }
    }
    preferences.putFloat("gmbx", bias.x);
    preferences.putFloat("gmby", bias.y);
    preferences.putFloat("gmbz", bias.z);

    preferences.end();
}

void dataFetchingTask(void* parameter) {
    Adafruit_MPU6050 mpu;
    mpu.begin();
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
    unsigned long lastReconnectAttempt = 0;
    const unsigned long RECONNECT_INTERVAL = 5000; // 5 seconds

    while (true) {
        int gaugeIndex;
        xSemaphoreTake(gaugeMutex, portMAX_DELAY);
        gaugeIndex = screenManager.getCurrentGaugeIndex();
        xSemaphoreGive(gaugeMutex);

        Gauge* gauge = gauges[gaugeIndex];
        switch (gauge->getType()) {
            case Gauge::NEEDLE_GAUGE:
                if (obdConnected) {
                    double reading = commands.getReading(gaugeIndex);
                    NeedleGauge* ng = static_cast<NeedleGauge*>(gauge);
                    ng->setReading(reading);
                    vTaskDelay(100 / portTICK_PERIOD_MS); // 10Hz
                } else {
                    if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL) {
                        obdConnected = reconnectToOBD();
                        lastReconnectAttempt = millis();
                    }
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                break;
            case Gauge::DUAL_GAUGE:
                if (obdConnected) {
                    struct dualGaugeReading x = commands.getDualReading(); // Gets torque and horsepower
                    DualGauge* dg = static_cast<DualGauge*>(gauge);
                    dg->setReadings(x.readings[0], x.readings[1]);
                    vTaskDelay(100 / portTICK_PERIOD_MS); // 10Hz
                } else {
                    if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL) {
                        obdConnected = reconnectToOBD();
                        lastReconnectAttempt = millis();
                    }
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                break;
            case Gauge::G_METER:
                {
                    sensors_event_t accel, gyro, temp;
                    mpu.getEvent(&accel, &gyro, &temp);
                    GMeter* gm = static_cast<GMeter*>(gauge);
                    gm->setAccelData(&accel, &gyro, &temp);
                    vTaskDelay(20 / portTICK_PERIOD_MS); // 50Hz
                }
                break;
            case Gauge::ACCELERATION_METER:
                if (obdConnected) {
                    double speed = commands.getReading(5); // Speed for AccelerationMeter
                    AccelerationMeter* am = static_cast<AccelerationMeter*>(gauge);
                    am->setSpeed(speed);
                    vTaskDelay(100 / portTICK_PERIOD_MS); // 10Hz
                } else {
                    if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL) {
                        obdConnected = reconnectToOBD();
                        lastReconnectAttempt = millis();
                    }
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                break;
            case Gauge::QUADRANT_GAUGE:
                if (obdConnected) {
                    QuadrantGauge* qg = static_cast<QuadrantGauge*>(gauge);
                    for (int i = 0; i < 4; i++) {
                        int commandIndex = qg->getCommandIndexForQuadrant(i);
                        qg->setQuadrantReading(i, commands.getValueByCommandIndex(commandIndex));
                    }
                    vTaskDelay(150 / portTICK_PERIOD_MS);
                } else {
                    if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL) {
                        obdConnected = reconnectToOBD();
                        lastReconnectAttempt = millis();
                    }
                    vTaskDelay(150 / portTICK_PERIOD_MS);
                }
                break;
        }
    }
}

void loop() {
    static bool wasTouched = false;
    static bool ignoreHeldTouchInOptions = false;
    static bool waitingForReleaseAfterOptions = false;
    static bool justExitedOptions = false;
    static unsigned long touchStartTime = 0;
    const unsigned long LONG_PRESS_THRESHOLD = 1000; // 1 second
    const unsigned long DEBOUNCE_MS = 200;

    if (touch_touched()) {
        if (millis() - lastTouchTime > DEBOUNCE_MS) {
            lastTouchTime = millis();
            if (!wasTouched) {
                wasTouched = true;
                touchStartTime = millis();

                if (inOptionsScreen && !waitingForReleaseAfterOptions) {
                    ignoreHeldTouchInOptions = false;
                }
            }

            if (inOptionsScreen) {
                if (!ignoreHeldTouchInOptions) {
                    if (touch_last_x >= 0 && touch_last_y >= 0) {
                        if (!optionsScreen->handleTouch(touch_last_x, touch_last_y)) {
                            exitOptions();
                            justExitedOptions = true;
                            Serial.println("Just exited options screen");
                        }
                    }
                }
            } else {
                bool handledSelectorTouch = false;
                xSemaphoreTake(gaugeMutex, portMAX_DELAY);
                Gauge* current = screenManager.getCurrentGauge();
                if (current != nullptr && current->getType() == Gauge::QUADRANT_GAUGE) {
                    QuadrantGauge* qg = static_cast<QuadrantGauge*>(current);
                    if (qg->isSelectorVisible()) {
                        qg->handleTouch(touch_last_x, touch_last_y);
                        handledSelectorTouch = true;
                    }
                }
                xSemaphoreGive(gaugeMutex);

                if (!handledSelectorTouch && millis() - touchStartTime > LONG_PRESS_THRESHOLD) {
                    xSemaphoreTake(gaugeMutex, portMAX_DELAY);
                    current = screenManager.getCurrentGauge();
                    if (current != nullptr && current->getType() == Gauge::QUADRANT_GAUGE) {
                        QuadrantGauge* qg = static_cast<QuadrantGauge*>(current);
                        qg->openSelectorFromTouch(touch_last_x, touch_last_y);
                    } else {
                        showOptions();
                        ignoreHeldTouchInOptions = true;
                        waitingForReleaseAfterOptions = true;
                    }
                    xSemaphoreGive(gaugeMutex);
                    wasTouched = true;
                }
            }
        }
    } else if (wasTouched && !touch_touched()) {
        wasTouched = false;

        if (waitingForReleaseAfterOptions) {
            // Now we can allow interaction again
            waitingForReleaseAfterOptions = false;
            ignoreHeldTouchInOptions = false;
        }

        if (justExitedOptions) {
            justExitedOptions = false;
            return;
        }

        if (!inOptionsScreen && millis() - touchStartTime < LONG_PRESS_THRESHOLD) {
            bool handledByQuadrantSelector = false;
            xSemaphoreTake(gaugeMutex, portMAX_DELAY);
            Gauge* current = screenManager.getCurrentGauge();
            if (current != nullptr && current->getType() == Gauge::QUADRANT_GAUGE) {
                QuadrantGauge* qg = static_cast<QuadrantGauge*>(current);
                handledByQuadrantSelector = qg->isSelectorVisible();
            }
            xSemaphoreGive(gaugeMutex);

            if (!handledByQuadrantSelector) {
                if ((touch_last_x >= 0 && touch_last_x < 30) &&
                    (touch_last_y > 210 && touch_last_y <= 240)) {
                    resetGauge();
                } else {
                    switchToNextGauge();
                }
            }
        }
    }

    // Render
    if (!inOptionsScreen) {
        xSemaphoreTake(gaugeMutex, portMAX_DELAY);
        Gauge* current = screenManager.getCurrentGauge();
        if (current != nullptr) {
            current->render(0.0);
        }

        GMeter* gm = static_cast<GMeter*>(gauges[5]);
        float savedRotation[3][3];
        Vec3 savedBias;
        if (gm->consumeCalibration(savedRotation, savedBias)) {
            updateGMeterCalibrationSettings(savedRotation, savedBias);
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    mpuCalibrationMatrix[i][j] = savedRotation[i][j];
                }
            }
            mpuCalibrationBias = savedBias;
            gmeterCalibrationStored = true;
        }

        xSemaphoreGive(gaugeMutex);
        //if (obdConnected) {
            //display.drawRect(0, 0, 10, 10, TFT_GREEN);
        //}
        //else {
            //display.drawRect(0, 0, 10, 10, TFT_RED);
        //}
    }

    // Limit to ~200 FPS
    //delay(250);
}

void switchToNextGauge() {
    xSemaphoreTake(gaugeMutex, portMAX_DELAY);
    screenManager.moveNext(obdConnected, FALLBACK_GAUGE_INDEX);
    currentGauge = screenManager.getCurrentGaugeIndex();
    Gauge* current = screenManager.getCurrentGauge();
    if (current != nullptr) {
        current->initialize();
    }

    updateCurrentGaugeSettings(currentGauge);
    xSemaphoreGive(gaugeMutex);
}

void resetGauge() {
    xSemaphoreTake(gaugeMutex, portMAX_DELAY);
    Gauge* current = screenManager.getCurrentGauge();
    if (current != nullptr) {
        current->reset();
    }
    xSemaphoreGive(gaugeMutex);
}

void showOptions() {
    inOptionsScreen = true;
    optionsScreen = new OptionsScreen(&display, gauges, GAUGE_COUNT);
    optionsScreen->initialize();
}

void exitOptions() {
    if (optionsScreen) {
        delete optionsScreen;
        optionsScreen = nullptr;
    }
    inOptionsScreen = false;
    xSemaphoreTake(gaugeMutex, portMAX_DELAY);
    Gauge* current = screenManager.getCurrentGauge();
    if (current == nullptr) {
        xSemaphoreGive(gaugeMutex);
        return;
    }
    current->initialize();

    Gauge::GaugeType gaugeType = current->getType();

    if (gaugeType != Gauge::ACCELERATION_METER && gaugeType != Gauge::G_METER) {
        // Store new preferences when exiting options screen only for needle and dual gauges
        uint32_t currNeedleColor = current->getCurrentNeedleColor();
        uint32_t currOutlineColor = current->getCurrentOutlineColor();
        uint32_t currValueColor = current->getCurrentValueColor();
        updateCurrentGaugeColorSettings(currNeedleColor, currOutlineColor, currValueColor);
    }

    xSemaphoreGive(gaugeMutex);
}