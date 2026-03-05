#ifndef TOUCH_H
#define TOUCH_H

#include <FT6336.h>

#define TOUCH_FT6336
#define TOUCH_FT6336_SCL 22
#define TOUCH_FT6336_SDA 21
#define TOUCH_FT6336_INT 15
#define TOUCH_FT6336_RST 13
#define TOUCH_MAP_X1 0
#define TOUCH_MAP_X2 240
#define TOUCH_MAP_Y1 0
#define TOUCH_MAP_Y2 320

struct TouchPoint {
  uint16_t x;
  uint16_t y;
  bool valid;
};

struct TouchSample {
  bool touched;
  uint8_t pointCount;
  TouchPoint points[2];
  uint32_t timestamp;
};

int touch_last_x = 0, touch_last_y = 0;
unsigned short int width = 0, height = 0, rotation, min_x = 0, max_x = 0, min_y = 0, max_y = 0;

FT6336 ts = FT6336(TOUCH_FT6336_SDA, TOUCH_FT6336_SCL, TOUCH_FT6336_INT, TOUCH_FT6336_RST,
                   max(TOUCH_MAP_X1, TOUCH_MAP_X2), max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));

static inline uint16_t mapTouchX(uint16_t rawX) {
  return map(rawX, min_x, max_x, 0, width - 1);
}

static inline uint16_t mapTouchY(uint16_t rawY) {
  return map(rawY, min_y, max_y, 0, height - 1);
}

void touch_init(unsigned short int w, unsigned short int h, unsigned char r) {
  width = w;
  height = h;
  rotation = r;

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

TouchSample touch_read_sample(void) {
  TouchSample sample;
  sample.touched = false;
  sample.pointCount = 0;
  sample.timestamp = millis();
  sample.points[0] = {0, 0, false};
  sample.points[1] = {0, 0, false};

  ts.read();

  if (!ts.isTouched) {
    return sample;
  }

  sample.touched = true;

  // Primary point is always available when touched.
  sample.points[0].x = mapTouchX(ts.points[0].x);
  sample.points[0].y = mapTouchY(ts.points[0].y);
  sample.points[0].valid = true;
  sample.pointCount = 1;

  touch_last_x = sample.points[0].x;
  touch_last_y = sample.points[0].y;

  // FT6336 supports up to two points. Treat a non-zero secondary point as valid.
  if (ts.points[1].x != 0 || ts.points[1].y != 0) {
    sample.points[1].x = mapTouchX(ts.points[1].x);
    sample.points[1].y = mapTouchY(ts.points[1].y);
    sample.points[1].valid = true;
    sample.pointCount = 2;
  }

  return sample;
}

bool touch_touched(void) {
  TouchSample sample = touch_read_sample();
  return sample.touched;
}

bool touch_getXY(uint16_t* x, uint16_t* y) {
  TouchSample sample = touch_read_sample();
  if (sample.touched && sample.points[0].valid) {
    *x = sample.points[0].x;
    *y = sample.points[0].y;
    return true;
  }
  return false;
}

bool touch_has_signal(void) {
  return true;
}

bool touch_released(void) {
  return true;
}

#endif
