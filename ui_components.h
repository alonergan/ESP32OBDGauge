#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include <TFT_eSPI.h>
#include <string.h>

struct UIRect {
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;

  bool contains(uint16_t px, uint16_t py) const {
    return px >= x && px < (x + w) && py >= y && py < (y + h);
  }
};

class UIButton {
public:
  UIButton() : label(""), textColor(TFT_WHITE), bgColor(TFT_DARKGREY), borderColor(TFT_WHITE), bounds({0, 0, 0, 0}) {}
  UIButton(UIRect rect, const char* text, uint16_t background, uint16_t border = TFT_WHITE, uint16_t textClr = TFT_WHITE)
      : label(text), textColor(textClr), bgColor(background), borderColor(border), bounds(rect) {}

  void setBounds(const UIRect& rect) { bounds = rect; }
  void setLabel(const char* text) { label = text; }
  void setColors(uint16_t background, uint16_t border = TFT_WHITE, uint16_t textClr = TFT_WHITE) {
    bgColor = background;
    borderColor = border;
    textColor = textClr;
  }

  bool hitTest(uint16_t x, uint16_t y) const { return bounds.contains(x, y); }

  void draw(TFT_eSprite& sprite, uint8_t font = 2, uint8_t textSize = 1) const {
    sprite.fillRect(bounds.x, bounds.y, bounds.w, bounds.h, bgColor);
    sprite.drawRect(bounds.x, bounds.y, bounds.w, bounds.h, borderColor);
    sprite.setTextFont(font);
    sprite.setTextSize(textSize);
    sprite.setTextColor(textColor);
    int16_t tw = sprite.textWidth(label);
    int16_t th = sprite.fontHeight();
    sprite.setCursor(bounds.x + (bounds.w - tw) / 2, bounds.y + (bounds.h - th) / 2);
    sprite.print(label);
  }

private:
  const char* label;
  uint16_t textColor;
  uint16_t bgColor;
  uint16_t borderColor;
  UIRect bounds;
};

class UIButtonGrid {
public:
  static const uint8_t MAX_ITEMS = 8;

  UIButtonGrid() : count(0) {}

  void configure(UIRect container, uint8_t columns, uint8_t rows, int16_t spacing) {
    if (columns == 0 || rows == 0) return;

    const int16_t cellW = (container.w - ((columns - 1) * spacing)) / columns;
    const int16_t cellH = (container.h - ((rows - 1) * spacing)) / rows;

    count = 0;
    for (uint8_t r = 0; r < rows; r++) {
      for (uint8_t c = 0; c < columns; c++) {
        if (count >= MAX_ITEMS) return;
        bounds[count] = {(int16_t)(container.x + c * (cellW + spacing)),
                         (int16_t)(container.y + r * (cellH + spacing)),
                         cellW,
                         cellH};
        buttons[count].setBounds(bounds[count]);
        buttons[count].setColors(TFT_DARKGREY);
        count++;
      }
    }
  }

  void setButton(uint8_t index, const char* label, uint16_t bgColor = TFT_DARKGREY) {
    if (index >= count) return;
    buttons[index].setBounds(bounds[index]);
    buttons[index].setLabel(label);
    buttons[index].setColors(bgColor);
  }

  int8_t hitTest(uint16_t x, uint16_t y) const {
    for (uint8_t i = 0; i < count; i++) {
      if (buttons[i].hitTest(x, y)) return (int8_t)i;
    }
    return -1;
  }

  void draw(TFT_eSprite& sprite) const {
    for (uint8_t i = 0; i < count; i++) buttons[i].draw(sprite);
  }

private:
  UIButton buttons[MAX_ITEMS];
  UIRect bounds[MAX_ITEMS];
  uint8_t count;
};

class UITable {
public:
  static const uint8_t MAX_ROWS = 10;

  UITable() : rowCount(0), rowHeight(22), bounds({0, 0, 0, 0}), headerColor(TFT_DARKGREY) { header[0] = '\0'; }

  void setBounds(const UIRect& rect) { bounds = rect; }
  void setHeader(const char* text, uint16_t color = TFT_DARKGREY) {
    strncpy(header, text, sizeof(header) - 1);
    header[sizeof(header) - 1] = '\0';
    headerColor = color;
  }
  void clearRows() { rowCount = 0; }

  void addRow(const char* key, const char* value) {
    if (rowCount >= MAX_ROWS) return;
    strncpy(rows[rowCount].key, key, sizeof(rows[rowCount].key) - 1);
    rows[rowCount].key[sizeof(rows[rowCount].key) - 1] = '\0';
    strncpy(rows[rowCount].value, value, sizeof(rows[rowCount].value) - 1);
    rows[rowCount].value[sizeof(rows[rowCount].value) - 1] = '\0';
    rowCount++;
  }

  void draw(TFT_eSprite& sprite) const {
    sprite.drawRect(bounds.x, bounds.y, bounds.w, bounds.h, TFT_WHITE);
    sprite.fillRect(bounds.x, bounds.y, bounds.w, rowHeight, headerColor);
    sprite.setTextFont(2);
    sprite.setTextSize(1);
    sprite.setTextColor(TFT_WHITE);
    sprite.setCursor(bounds.x + 6, bounds.y + 4);
    sprite.print(header);

    for (uint8_t i = 0; i < rowCount; i++) {
      int16_t rowY = bounds.y + rowHeight + (i * rowHeight);
      if (rowY + rowHeight > bounds.y + bounds.h) break;
      sprite.drawLine(bounds.x, rowY, bounds.x + bounds.w, rowY, TFT_DARKGREY);
      sprite.setTextColor(TFT_CYAN);
      sprite.setCursor(bounds.x + 6, rowY + 4);
      sprite.print(rows[i].key);
      sprite.setTextColor(TFT_WHITE);
      sprite.setCursor(bounds.x + (bounds.w / 2), rowY + 4);
      sprite.print(rows[i].value);
    }
  }

private:
  struct TableRow { char key[18]; char value[22]; };

  TableRow rows[MAX_ROWS];
  uint8_t rowCount;
  int16_t rowHeight;
  UIRect bounds;
  char header[24];
  uint16_t headerColor;
};

#endif
