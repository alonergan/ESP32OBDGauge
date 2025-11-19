#ifndef GAUGE_H
#define GAUGE_H

#include <TFT_eSPI.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "config.h"

class Gauge {
public:
    enum GaugeType {
        NEEDLE_GAUGE,
        DUAL_GAUGE,
        G_METER,
        ACCELERATION_METER
    };

    Gauge(TFT_eSPI* display) : display(display) {}
    virtual void render(double reading) = 0;
    virtual void initialize() = 0;
    virtual void displayStats(float fps, double frameAvg, double queryAvg) = 0;
    virtual GaugeType getType() const = 0;
    virtual void reset() = 0;
    virtual uint32_t getCurrentNeedleColor() = 0;
    virtual uint32_t getCurrentOutlineColor() = 0;
    virtual uint32_t getCurrentValueColor() = 0;

protected:
    TFT_eSPI* display;
};

#endif