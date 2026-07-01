#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include "gauge.h"

class ScreenManager {
public:
    ScreenManager() : currentGaugeIndex(0), gaugeCount(0), gauges(nullptr) {}

    void initialize(Gauge** gaugeList, int count, int initialIndex) {
        gauges = gaugeList;
        gaugeCount = count;
        currentGaugeIndex = constrainIndex(initialIndex);
    }

    int getCurrentGaugeIndex() const {
        return currentGaugeIndex;
    }

    Gauge* getCurrentGauge() const {
        if (gauges == nullptr || gaugeCount == 0) {
            return nullptr;
        }
        return gauges[currentGaugeIndex];
    }

    void setCurrentGauge(int newIndex) {
        currentGaugeIndex = constrainIndex(newIndex);
    }

    void moveNext(bool obdConnected, int fallbackGaugeIndex) {
        if (gaugeCount == 0) {
            return;
        }

        if (obdConnected) {
            currentGaugeIndex = (currentGaugeIndex + 1) % gaugeCount;
        } else {
            currentGaugeIndex = constrainIndex(fallbackGaugeIndex);
        }
    }

private:
    int currentGaugeIndex;
    int gaugeCount;
    Gauge** gauges;

    int constrainIndex(int index) const {
        if (gaugeCount <= 0) {
            return 0;
        }
        if (index < 0) {
            return 0;
        }
        if (index >= gaugeCount) {
            return gaugeCount - 1;
        }
        return index;
    }
};

#endif
