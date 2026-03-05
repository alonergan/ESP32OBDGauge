#ifndef TOUCH_H
#define TOUCH_H

#include <FT6336.h>
#include <math.h>

#define TOUCH_FT6336
#define TOUCH_FT6336_SCL 22
#define TOUCH_FT6336_SDA 21
#define TOUCH_FT6336_INT 15
#define TOUCH_FT6336_RST 13
#define TOUCH_MAP_X1 0
#define TOUCH_MAP_X2 240
#define TOUCH_MAP_Y1 0
#define TOUCH_MAP_Y2 320

struct TouchGesture {
  enum Type {
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

  Type type;
  int16_t startX;
  int16_t startY;
  int16_t endX;
  int16_t endY;
  int16_t deltaX;
  int16_t deltaY;
  float pinchScale;
};

int touch_last_x = 0, touch_last_y = 0;
unsigned short int width = 0, height = 0, rotation, min_x = 0, max_x = 0, min_y = 0, max_y = 0;

FT6336 ts = FT6336(TOUCH_FT6336_SDA, TOUCH_FT6336_SCL, TOUCH_FT6336_INT, TOUCH_FT6336_RST, max(TOUCH_MAP_X1, TOUCH_MAP_X2), max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));

static bool touch_active = false;
static bool touch_has_second = false;
static int16_t touch_start_x = 0;
static int16_t touch_start_y = 0;
static unsigned long touch_start_time = 0;
static float pinch_start_distance = 0.0f;
static TouchGesture pendingGesture = {TouchGesture::NONE, 0, 0, 0, 0, 0, 0, 1.0f};

inline int16_t mapTouchX(int rawX) {
  return map(rawX, min_x, max_x, 0, width - 1);
}

inline int16_t mapTouchY(int rawY) {
  return map(rawY, min_y, max_y, 0, height - 1);
}

inline float touchDistance(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
  float dx = static_cast<float>(x2 - x1);
  float dy = static_cast<float>(y2 - y1);
  return sqrtf((dx * dx) + (dy * dy));
}

void touch_init(unsigned short int w, unsigned short int h, unsigned char r) {
  width = w;
  height = h;
  switch (r) {
    case ROTATION_NORMAL:
    case ROTATION_INVERTED:
      min_x = TOUCH_MAP_X1;
      max_x = TOUCH_MAP_X2;
      min_y = TOUCH_MAP_Y1;
      max_y = TOUCH_MAP_Y2;
      break;
    case ROTATION_LEFT:
    case ROTATION_RIGHT:
      min_x = TOUCH_MAP_Y1;
      max_x = TOUCH_MAP_Y2;
      min_y = TOUCH_MAP_X1;
      max_y = TOUCH_MAP_X2;
      break;
    default:
      break;
  }
  ts.begin();
  ts.setRotation(r);
}

bool touch_touched(void) {
  ts.read();
  if (ts.isTouched) {
    touch_last_x = mapTouchX(ts.points[0].x);
    touch_last_y = mapTouchY(ts.points[0].y);

    bool secondTouch = (ts.points[1].x > 0 || ts.points[1].y > 0);

    if (!touch_active) {
      touch_active = true;
      touch_has_second = secondTouch;
      touch_start_x = touch_last_x;
      touch_start_y = touch_last_y;
      touch_start_time = millis();

      if (secondTouch) {
        int16_t x2 = mapTouchX(ts.points[1].x);
        int16_t y2 = mapTouchY(ts.points[1].y);
        pinch_start_distance = touchDistance(touch_last_x, touch_last_y, x2, y2);
      } else {
        pinch_start_distance = 0.0f;
      }
    }

    Serial.printf("Touched at x: %.1d. y: %.1d\n", touch_last_x, touch_last_y);
    return true;
  }

  if (touch_active) {
    unsigned long duration = millis() - touch_start_time;
    int16_t dx = touch_last_x - touch_start_x;
    int16_t dy = touch_last_y - touch_start_y;
    int16_t adx = abs(dx);
    int16_t ady = abs(dy);

    pendingGesture = {TouchGesture::NONE, touch_start_x, touch_start_y, static_cast<int16_t>(touch_last_x), static_cast<int16_t>(touch_last_y), dx, dy, 1.0f};

    if (touch_has_second && pinch_start_distance > 1.0f) {
      float currentDistance = pinch_start_distance;
      if (ts.points[1].x > 0 || ts.points[1].y > 0) {
        int16_t x2 = mapTouchX(ts.points[1].x);
        int16_t y2 = mapTouchY(ts.points[1].y);
        currentDistance = touchDistance(touch_last_x, touch_last_y, x2, y2);
      }
      float scale = currentDistance / pinch_start_distance;
      pendingGesture.pinchScale = scale;
      if (scale > 1.20f) {
        pendingGesture.type = TouchGesture::PINCH_OUT;
      } else if (scale < 0.85f) {
        pendingGesture.type = TouchGesture::PINCH_IN;
      }
    }

    if (pendingGesture.type == TouchGesture::NONE) {
      if (adx > 50 || ady > 50) {
        if (adx > ady) {
          pendingGesture.type = dx > 0 ? TouchGesture::SWIPE_RIGHT : TouchGesture::SWIPE_LEFT;
        } else {
          pendingGesture.type = dy > 0 ? TouchGesture::SWIPE_DOWN : TouchGesture::SWIPE_UP;
        }
      } else if (duration > 1000) {
        pendingGesture.type = TouchGesture::LONG_PRESS;
      } else {
        pendingGesture.type = TouchGesture::TAP;
      }
    }

    touch_active = false;
    touch_has_second = false;
    pinch_start_distance = 0.0f;
  }

  return false;
}

bool touch_getXY(uint16_t* x, uint16_t* y) {
  if (touch_touched()) {
    *x = touch_last_x;
    *y = touch_last_y;
    return true;
  }
  return false;
}

bool touch_getGesture(TouchGesture* gesture) {
  if (pendingGesture.type == TouchGesture::NONE || gesture == nullptr) {
    return false;
  }

  *gesture = pendingGesture;
  pendingGesture.type = TouchGesture::NONE;
  pendingGesture.pinchScale = 1.0f;
  return true;
}

bool touch_has_signal(void) {
  return true;
}

bool touch_released(void) {
  return !touch_active;
}

#endif
