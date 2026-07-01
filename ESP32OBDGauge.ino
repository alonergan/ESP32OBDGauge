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
#include "config.h"
#include "options_screen.h"
#include "screen_manager.h"

bool TESTMODE = true;

/*
    Variables for settings stored in flash memory
*/
uint32_t outlineColor = GAUGE_FG_COLOR;
uint32_t labelColor = GAUGE_FG_COLOR;
uint32_t valueColor = NEEDLE_COLOR_PRIMARY;
int currentGauge = 0;
int singleCommandIndex = 4;
int dualLeftCommandIndex = 16;
int dualRightCommandIndex = 18;
int quadrantCommandIndices[4] = {4, 5, 1, 17};
uint16_t favoriteColors[8] = {TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_BLUE, TFT_PURPLE, TFT_WHITE};
float mpuCalibrationMatrix[3][3]; // Persisted G-meter rotation matrix
Vec3 mpuCalibrationBias = {0.0f, 0.0f, 0.0f};
bool gmeterCalibrationStored = false;

Preferences preferences;
TFT_eSPI display = TFT_eSPI();
const int GAUGE_COUNT = 5;
const int FALLBACK_GAUGE_INDEX = 2;
uint32_t gaugeOutlineColors[GAUGE_COUNT] = {GAUGE_FG_COLOR, GAUGE_FG_COLOR, GAUGE_FG_COLOR, GAUGE_FG_COLOR, GAUGE_FG_COLOR};
uint32_t gaugeLabelColors[GAUGE_COUNT] = {GAUGE_FG_COLOR, GAUGE_FG_COLOR, GAUGE_FG_COLOR, GAUGE_FG_COLOR, GAUGE_FG_COLOR};
uint32_t gaugeValueColors[GAUGE_COUNT] = {NEEDLE_COLOR_PRIMARY, NEEDLE_COLOR_PRIMARY, NEEDLE_COLOR_PRIMARY, NEEDLE_COLOR_PRIMARY, NEEDLE_COLOR_PRIMARY};
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
    gauges[0] = new NeedleGauge(&display, &commands, singleCommandIndex, gaugeOutlineColors[0], gaugeLabelColors[0], gaugeValueColors[0]);
    gauges[1] = new DualGauge(&display, &commands, dualLeftCommandIndex, dualRightCommandIndex, gaugeOutlineColors[1], gaugeLabelColors[1], gaugeValueColors[1]);
    gauges[2] = new GMeter(&display, gaugeOutlineColors[2], gaugeLabelColors[2], gaugeValueColors[2]);
    gauges[3] = new AccelerationMeter(&display, gaugeOutlineColors[3], gaugeLabelColors[3], gaugeValueColors[3]);
    gauges[4] = new QuadrantGauge(&display, &commands, quadrantCommandIndices, gaugeOutlineColors[4], gaugeLabelColors[4], gaugeValueColors[4]);

    if (gmeterCalibrationStored) {
        static_cast<GMeter*>(gauges[2])->setStoredCalibration(mpuCalibrationMatrix, mpuCalibrationBias);
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
    labelColor = preferences.getUInt("labelColor", GAUGE_FG_COLOR);
    valueColor = preferences.getUInt("valueColor", preferences.getUInt("needleColor", NEEDLE_COLOR_PRIMARY));
    for (int i = 0; i < GAUGE_COUNT; i++) {
        String labelKey = "lbl" + String(i);
        String valueKey = "val" + String(i);
        String outlineKey = "out" + String(i);
        gaugeLabelColors[i] = preferences.getUInt(labelKey.c_str(), labelColor);
        gaugeValueColors[i] = preferences.getUInt(valueKey.c_str(), valueColor);
        gaugeOutlineColors[i] = preferences.getUInt(outlineKey.c_str(), outlineColor);
    }
    currentGauge = preferences.getInt("currentGauge", 0);
    int commandCount = commands.getCommandCount();
    singleCommandIndex = constrain(preferences.getInt("singleCmd", 4), 0, commandCount - 1);
    dualLeftCommandIndex = constrain(preferences.getInt("dualLeft", 16), 0, commandCount - 1);
    dualRightCommandIndex = constrain(preferences.getInt("dualRight", 18), 0, commandCount - 1);
    for (int i = 0; i < 4; i++) {
        String key = "quad" + String(i);
        quadrantCommandIndices[i] = constrain(preferences.getInt(key.c_str(), quadrantCommandIndices[i]), 0, commandCount - 1);
    }
    for (int i = 0; i < 8; i++) {
        String key = "fav" + String(i);
        favoriteColors[i] = static_cast<uint16_t>(preferences.getUInt(key.c_str(), favoriteColors[i]));
    }

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

void updateGaugeConfigurationSettings() {
    // Initialize preferences
    preferences.begin("OBDGAUGE", false);

    // Preserve the original keys as migration defaults, then store each
    // screen's colors independently.
    preferences.putUInt("labelColor", gauges[0]->getCurrentLabelColor());
    preferences.putUInt("outlineColor", gauges[0]->getCurrentOutlineColor());
    preferences.putUInt("valueColor", gauges[0]->getCurrentValueColor());
    for (int i = 0; i < GAUGE_COUNT; i++) {
        String labelKey = "lbl" + String(i);
        String valueKey = "val" + String(i);
        String outlineKey = "out" + String(i);
        preferences.putUInt(labelKey.c_str(), gauges[i]->getCurrentLabelColor());
        preferences.putUInt(valueKey.c_str(), gauges[i]->getCurrentValueColor());
        preferences.putUInt(outlineKey.c_str(), gauges[i]->getCurrentOutlineColor());
    }

    NeedleGauge* single = static_cast<NeedleGauge*>(gauges[0]);
    DualGauge* dual = static_cast<DualGauge*>(gauges[1]);
    QuadrantGauge* quadrant = static_cast<QuadrantGauge*>(gauges[4]);
    preferences.putInt("singleCmd", single->getCommandIndex());
    preferences.putInt("dualLeft", dual->getLeftCommandIndex());
    preferences.putInt("dualRight", dual->getRightCommandIndex());
    for (int i = 0; i < 4; i++) {
        String key = "quad" + String(i);
        preferences.putInt(key.c_str(), quadrant->getCommandIndexForQuadrant(i));
    }
    for (int i = 0; i < 8; i++) {
        String key = "fav" + String(i);
        preferences.putUInt(key.c_str(), favoriteColors[i]);
    }

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
                    NeedleGauge* ng = static_cast<NeedleGauge*>(gauge);
                    double reading = commands.getValueByCommandIndex(ng->getCommandIndex());
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
                    DualGauge* dg = static_cast<DualGauge*>(gauge);
                    double left = commands.getValueByCommandIndex(dg->getLeftCommandIndex());
                    double right = commands.getValueByCommandIndex(dg->getRightCommandIndex());
                    dg->setReadings(left, right);
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
                    am->setSpeed(speed, commands.wasLastQuerySuccessful());
                    vTaskDelay(100 / portTICK_PERIOD_MS); // 10Hz
                } else {
                    static_cast<AccelerationMeter*>(gauge)->setSpeed(0.0, false);
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

void switchToPreviousGauge() {
    xSemaphoreTake(gaugeMutex, portMAX_DELAY);
    if (obdConnected) {
        int prev = screenManager.getCurrentGaugeIndex() - 1;
        if (prev < 0) {
            prev = GAUGE_COUNT - 1;
        }
        screenManager.setCurrentGauge(prev);
    } else {
        screenManager.setCurrentGauge(FALLBACK_GAUGE_INDEX);
    }

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
    optionsScreen = new OptionsScreen(&display, gauges, GAUGE_COUNT, &commands, favoriteColors,
                                      &obdConnected, screenManager.getCurrentGaugeIndex());
    optionsScreen->initialize();
}

void showGaugeTypeOptions(Gauge::GaugeType gaugeType, int16_t touchX) {
    showOptions();
    optionsScreen->openGaugeTypeShortcut(gaugeType, touchX);
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

    updateGaugeConfigurationSettings();

    xSemaphoreGive(gaugeMutex);
}

void loop() {
    static bool wasTouched = false;
    static bool waitingForReleaseAfterOptions = false;
    static bool waitingForReleaseAfterQuadrantSelector = false;
    static bool touchHandledForCurrentPress = false;
    static unsigned long touchStartTime = 0;
    static unsigned long lastGaugeTapTime = 0;
    static int lastTapGaugeIndex = -1;
    static int lastTapRegion = -1;
    const unsigned long LONG_PRESS_THRESHOLD = 1000; // 1 second
    const unsigned long DOUBLE_TAP_THRESHOLD = 350;
    const unsigned long DEBOUNCE_MS = 200;

    bool currentlyTouched = touch_touched();

    if (currentlyTouched) {
        if (millis() - lastTouchTime > DEBOUNCE_MS) {
            lastTouchTime = millis();
            if (!wasTouched) {
                wasTouched = true;
                touchStartTime = millis();
                touchHandledForCurrentPress = false;
            }

            if (!inOptionsScreen) {
                bool handledSelectorTouch = false;
                xSemaphoreTake(gaugeMutex, portMAX_DELAY);
                Gauge* current = screenManager.getCurrentGauge();
                if (current != nullptr && current->getType() == Gauge::QUADRANT_GAUGE) {
                    QuadrantGauge* qg = static_cast<QuadrantGauge*>(current);
                    if (qg->isSelectorVisible()) {
                        if (!touchHandledForCurrentPress) {
                            qg->handleTouch(touch_last_x, touch_last_y);
                            if (!qg->isSelectorVisible()) updateGaugeConfigurationSettings();
                            touchHandledForCurrentPress = true;
                            waitingForReleaseAfterQuadrantSelector = true;
                        }
                        handledSelectorTouch = true;
                    }
                }
                xSemaphoreGive(gaugeMutex);

                if (!handledSelectorTouch && millis() - touchStartTime > LONG_PRESS_THRESHOLD) {
                    showOptions();
                    waitingForReleaseAfterOptions = true;
                    lastGaugeTapTime = 0;
                    lastTapGaugeIndex = -1;
                    lastTapRegion = -1;
                    wasTouched = true;
                    touchHandledForCurrentPress = true;
                }
            }
        }
    } else if (wasTouched) {
        wasTouched = false;

        if (waitingForReleaseAfterOptions) {
            // Now we can allow interaction again
            waitingForReleaseAfterOptions = false;
        }
    }

    TouchGesture gesture;
    if (touch_getGesture(&gesture)) {
        if (inOptionsScreen) {
            if (!optionsScreen->handleTouchGesture(gesture)) {
                exitOptions();
            }
            touchHandledForCurrentPress = false;
        } else {
            if (gesture.type == TouchGesture::TAP) {
                bool handledTap = false;
                bool openGaugeTypeOptions = false;
                Gauge::GaugeType shortcutType = Gauge::NEEDLE_GAUGE;
                xSemaphoreTake(gaugeMutex, portMAX_DELAY);
                Gauge* current = screenManager.getCurrentGauge();
                if (current != nullptr && current->getType() == Gauge::QUADRANT_GAUGE) {
                    QuadrantGauge* qg = static_cast<QuadrantGauge*>(current);
                    if (qg->isSelectorVisible()) {
                        qg->handleTouch(gesture.endX, gesture.endY);
                        if (!qg->isSelectorVisible()) updateGaugeConfigurationSettings();
                        handledTap = true;
                    }
                }

                if (current != nullptr && !handledTap) {
                    int gaugeIndex = screenManager.getCurrentGaugeIndex();
                    int tapRegion = 0;
                    if (current->getType() == Gauge::QUADRANT_GAUGE) {
                        tapRegion = (gesture.endX >= DISPLAY_WIDTH / 2 ? 1 : 0) +
                                    (gesture.endY >= DISPLAY_HEIGHT / 2 ? 2 : 0);
                    } else if (current->getType() == Gauge::DUAL_GAUGE) {
                        tapRegion = gesture.endX < DISPLAY_CENTER_X ? 0 : 1;
                    }

                    unsigned long now = millis();
                    if (gaugeIndex == lastTapGaugeIndex && tapRegion == lastTapRegion &&
                        now - lastGaugeTapTime <= DOUBLE_TAP_THRESHOLD) {
                        if (current->getType() == Gauge::QUADRANT_GAUGE) {
                            static_cast<QuadrantGauge*>(current)->openSelectorFromTouch(gesture.endX, gesture.endY);
                        } else {
                            shortcutType = current->getType();
                            openGaugeTypeOptions = true;
                        }
                        lastGaugeTapTime = 0;
                        lastTapGaugeIndex = -1;
                        lastTapRegion = -1;
                    } else {
                        lastGaugeTapTime = now;
                        lastTapGaugeIndex = gaugeIndex;
                        lastTapRegion = tapRegion;
                    }
                    handledTap = true;
                }
                xSemaphoreGive(gaugeMutex);

                if (openGaugeTypeOptions) showGaugeTypeOptions(shortcutType, gesture.endX);

                if (handledTap || touchHandledForCurrentPress) {
                    touchHandledForCurrentPress = false;
                    return;
                }
            } else if (gesture.type == TouchGesture::SWIPE_LEFT) {
                switchToNextGauge();
            } else if (gesture.type == TouchGesture::SWIPE_RIGHT) {
                xSemaphoreTake(gaugeMutex, portMAX_DELAY);
                int previousGauge = screenManager.getCurrentGaugeIndex() - 1;
                if (previousGauge < 0) {
                    previousGauge = GAUGE_COUNT - 1;
                }
                screenManager.setCurrentGauge(previousGauge);
                currentGauge = screenManager.getCurrentGaugeIndex();
                Gauge* current = screenManager.getCurrentGauge();
                if (current != nullptr) {
                    current->initialize();
                }
                updateCurrentGaugeSettings(currentGauge);
                xSemaphoreGive(gaugeMutex);
            } else if (gesture.type == TouchGesture::SWIPE_DOWN) {
                resetGauge();
            }

            touchHandledForCurrentPress = false;
        }
    }

    if (!currentlyTouched) {
        if (waitingForReleaseAfterOptions) {
            waitingForReleaseAfterOptions = false;
        }

        if (waitingForReleaseAfterQuadrantSelector) {
            waitingForReleaseAfterQuadrantSelector = false;
            return;
        }
    }

    // Render
    if (!inOptionsScreen) {
        xSemaphoreTake(gaugeMutex, portMAX_DELAY);
        Gauge* current = screenManager.getCurrentGauge();
        if (current != nullptr) {
            current->render(0.0);
        }

        GMeter* gm = static_cast<GMeter*>(gauges[2]);
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
    }
}
