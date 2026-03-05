#include <TFT_eSPI.h>
#include "touch.h"
#include "bluetooth.h"
#include "commands.h"
#include "needle_gauge.h"
#include "g_meter.h"
#include "acceleration_meter.h"
#include "options_screen.h"
#include "config.h"

bool TESTMODE = true;

TFT_eSPI display = TFT_eSPI();
Gauge* gauges[6];
int currentGauge = 0;
TaskHandle_t dataTaskHandle;
SemaphoreHandle_t gaugeMutex;
unsigned long lastTouchTime = 0;
bool obdConnected = false;
OptionsScreen* optionsScreen = nullptr;
bool inOptionsScreen = false;

void setup() {
    Serial.begin(115200);
    display.begin();
    display.setRotation(1);
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
    gauges[0] = new NeedleGauge(&display, 0); // RPM    (Actual)
    gauges[1] = new NeedleGauge(&display, 1); // Boost  (Approximate)
    gauges[2] = new NeedleGauge(&display, 2); // Torque (Approximate)
    gauges[3] = new NeedleGauge(&display, 3); // Horsepower (Approximate)
    gauges[4] = new GMeter(&display);
    gauges[5] = new AccelerationMeter(&display);

    gauges[obdConnected ? 0 : 4]->initialize();
    if (!obdConnected) {
        currentGauge = 4;
    }

    xTaskCreatePinnedToCore(dataFetchingTask, "DataFetching", 10000, NULL, 1, &dataTaskHandle, 0);
}

void dataFetchingTask(void* parameter) {
    Commands commands;
    Adafruit_MPU6050 mpu;
    mpu.begin();
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
    unsigned long lastReconnectAttempt = 0;
    const unsigned long RECONNECT_INTERVAL = 5000; // 5 seconds

    while (true) {
        int gaugeIndex;
        xSemaphoreTake(gaugeMutex, portMAX_DELAY);
        gaugeIndex = currentGauge;
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
                    double speed = commands.getReading(3); // Speed for AccelerationMeter
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
            } else if (millis() - touchStartTime > LONG_PRESS_THRESHOLD) {
                showOptions();
                ignoreHeldTouchInOptions = true;
                waitingForReleaseAfterOptions = true;
                wasTouched = true;  // Don't reset — finger still down
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
            if ((touch_last_x >= 0 && touch_last_x < 30) &&
                (touch_last_y > 210 && touch_last_y <= 240)) {
                resetGauge();
            } else {
                switchToNextGauge();
            }
        }
    }

    // Render
    if (!inOptionsScreen) {
        xSemaphoreTake(gaugeMutex, portMAX_DELAY);
        gauges[currentGauge]->render(0.0);
        xSemaphoreGive(gaugeMutex);
        if (obdConnected) {
            display.drawRect(0, 0, 10, 10, TFT_GREEN);
        }
        else {
            display.drawRect(0, 0, 10, 10, TFT_RED);
        }
    }

    // Limit to ~100 FPS
    delay(10);
}

void switchToNextGauge() {
    xSemaphoreTake(gaugeMutex, portMAX_DELAY);
    if (obdConnected) {
        currentGauge = (currentGauge + 1) % 6;
    } else {
        currentGauge = 4; // Stay on GMeter if OBD not connected
    }
    gauges[currentGauge]->initialize();
    xSemaphoreGive(gaugeMutex);
}

void resetGauge() {
    xSemaphoreTake(gaugeMutex, portMAX_DELAY);
    gauges[currentGauge]->reset();
    xSemaphoreGive(gaugeMutex);
}

void showOptions() {
    inOptionsScreen = true;
    optionsScreen = new OptionsScreen(&display, gauges, 5);
    optionsScreen->initialize();
}

void exitOptions() {
    if (optionsScreen) {
        delete optionsScreen;
        optionsScreen = nullptr;
    }
    inOptionsScreen = false;
    xSemaphoreTake(gaugeMutex, portMAX_DELAY);
    gauges[currentGauge]->initialize();
    xSemaphoreGive(gaugeMutex);
}