#ifndef UI_TOUCH_H
#define UI_TOUCH_H

#include "touch.h"
#include <math.h>

class TouchGestureEngine {
public:
  enum GestureType {
    NONE,
    TAP,
    LONG_PRESS,
    SWIPE_LEFT,
    SWIPE_RIGHT,
    SWIPE_UP,
    SWIPE_DOWN,
    PINCH_IN,
    PINCH_OUT
  };

  struct GestureEvent {
    GestureType type;
    TouchSample sample;
  };

  TouchGestureEngine()
      : isTouchDown(false),
        touchStartMs(0),
        startX(0),
        startY(0),
        lastX(0),
        lastY(0),
        hasPinchStart(false),
        pinchStartDistance(0.0f),
        pinchCooldownUntil(0),
        longPressFired(false) {}

  GestureEvent update(const TouchSample& sample) {
    GestureEvent event = {NONE, sample};

    if (sample.touched && sample.pointCount > 0) {
      if (!isTouchDown) {
        beginTouch(sample);
      }

      lastX = sample.points[0].x;
      lastY = sample.points[0].y;

      if (!longPressFired && sample.pointCount == 1 &&
          (sample.timestamp - touchStartMs) > LONG_PRESS_MS) {
        longPressFired = true;
        event.type = LONG_PRESS;
        return event;
      }

      if (sample.pointCount >= 2 && sample.points[0].valid && sample.points[1].valid) {
        const float distanceNow = distanceBetween(sample.points[0], sample.points[1]);

        if (!hasPinchStart) {
          pinchStartDistance = distanceNow;
          hasPinchStart = true;
        } else if (sample.timestamp >= pinchCooldownUntil) {
          const float delta = distanceNow - pinchStartDistance;
          if (delta > PINCH_THRESHOLD) {
            pinchCooldownUntil = sample.timestamp + PINCH_COOLDOWN_MS;
            pinchStartDistance = distanceNow;
            event.type = PINCH_OUT;
            return event;
          }
          if (delta < -PINCH_THRESHOLD) {
            pinchCooldownUntil = sample.timestamp + PINCH_COOLDOWN_MS;
            pinchStartDistance = distanceNow;
            event.type = PINCH_IN;
            return event;
          }
        }
      }

      return event;
    }

    if (!sample.touched && isTouchDown) {
      event.type = classifyReleaseGesture(sample);
      resetTouchState();
      return event;
    }

    return event;
  }

private:
  static const uint16_t TAP_MAX_TRAVEL = 20;
  static const uint16_t SWIPE_MIN_TRAVEL = 70;
  static const uint16_t LONG_PRESS_MS = 900;
  static constexpr float PINCH_THRESHOLD = 18.0f;
  static const uint16_t PINCH_COOLDOWN_MS = 120;

  bool isTouchDown;
  uint32_t touchStartMs;
  int16_t startX;
  int16_t startY;
  int16_t lastX;
  int16_t lastY;
  bool hasPinchStart;
  float pinchStartDistance;
  uint32_t pinchCooldownUntil;
  bool longPressFired;

  void beginTouch(const TouchSample& sample) {
    isTouchDown = true;
    touchStartMs = sample.timestamp;
    startX = sample.points[0].x;
    startY = sample.points[0].y;
    lastX = startX;
    lastY = startY;
    hasPinchStart = false;
    longPressFired = false;
  }

  void resetTouchState() {
    isTouchDown = false;
    hasPinchStart = false;
    longPressFired = false;
  }

  GestureType classifyReleaseGesture(const TouchSample& sample) {
    if (longPressFired) {
      return NONE;
    }

    const int16_t dx = lastX - startX;
    const int16_t dy = lastY - startY;
    const uint16_t absDx = abs(dx);
    const uint16_t absDy = abs(dy);

    if (absDx <= TAP_MAX_TRAVEL && absDy <= TAP_MAX_TRAVEL) {
      return TAP;
    }

    if (absDx >= SWIPE_MIN_TRAVEL || absDy >= SWIPE_MIN_TRAVEL) {
      if (absDx > absDy) {
        return dx > 0 ? SWIPE_RIGHT : SWIPE_LEFT;
      }
      return dy > 0 ? SWIPE_DOWN : SWIPE_UP;
    }

    return NONE;
  }

  float distanceBetween(const TouchPoint& a, const TouchPoint& b) {
    const float dx = (float)a.x - (float)b.x;
    const float dy = (float)a.y - (float)b.y;
    return sqrtf((dx * dx) + (dy * dy));
  }
};

#endif
