#ifndef UI_LIBRARY_H
#define UI_LIBRARY_H

#include <TFT_eSPI.h>

struct UIRect {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;

    bool contains(int16_t px, int16_t py) const {
        return px >= x && px < (x + w) && py >= y && py < (y + h);
    }
};

class UIButton {
public:
    UIButton() : id(-1), label(""), visible(true), enabled(true), bgColor(TFT_DARKGREY), borderColor(TFT_WHITE), textColor(TFT_WHITE) {
        bounds = {0, 0, 0, 0};
    }

    UIButton(int buttonId, const char* buttonLabel, const UIRect& rect, uint16_t bg = TFT_DARKGREY, uint16_t border = TFT_WHITE, uint16_t text = TFT_WHITE)
        : id(buttonId), label(buttonLabel), bounds(rect), visible(true), enabled(true), bgColor(bg), borderColor(border), textColor(text) {}

    void draw(TFT_eSprite& sprite, uint8_t textFont = 2, uint8_t textSize = 1) const {
        if (!visible) {
            return;
        }

        sprite.fillRect(bounds.x, bounds.y, bounds.w, bounds.h, enabled ? bgColor : TFT_DARKGREY);
        sprite.drawRect(bounds.x, bounds.y, bounds.w, bounds.h, borderColor);
        sprite.setTextFont(textFont);
        sprite.setTextSize(textSize);
        sprite.setTextColor(textColor);

        int textWidth = sprite.textWidth(label);
        int textHeight = sprite.fontHeight();
        sprite.setCursor(bounds.x + (bounds.w - textWidth) / 2, bounds.y + (bounds.h - textHeight) / 2);
        sprite.print(label);
    }

    bool hitTest(int16_t px, int16_t py) const {
        return visible && enabled && bounds.contains(px, py);
    }

    int getId() const { return id; }
    const UIRect& getBounds() const { return bounds; }

private:
    int id;
    const char* label;
    UIRect bounds;
    bool visible;
    bool enabled;
    uint16_t bgColor;
    uint16_t borderColor;
    uint16_t textColor;
};

class UITable {
public:
    static const uint8_t MAX_ROWS = 6;
    static const uint8_t MAX_COLS = 3;

    UITable() : rows(0), cols(0), headerBg(TFT_DARKGREY), cellBg(TFT_BLACK), border(TFT_DARKGREY), text(TFT_WHITE) {
        bounds = {0, 0, 0, 0};
        for (int r = 0; r < MAX_ROWS; r++) {
            for (int c = 0; c < MAX_COLS; c++) {
                cellText[r][c] = "";
            }
        }
    }

    void configure(const UIRect& rect, uint8_t rowCount, uint8_t colCount) {
        bounds = rect;
        rows = rowCount > MAX_ROWS ? MAX_ROWS : rowCount;
        cols = colCount > MAX_COLS ? MAX_COLS : colCount;
    }

    void setCell(uint8_t row, uint8_t col, const char* value) {
        if (row < rows && col < cols) {
            cellText[row][col] = value;
        }
    }

    void draw(TFT_eSprite& sprite, bool hasHeader = true) const {
        if (rows == 0 || cols == 0) {
            return;
        }

        int cellW = bounds.w / cols;
        int cellH = bounds.h / rows;

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                int x = bounds.x + (c * cellW);
                int y = bounds.y + (r * cellH);
                uint16_t bg = (hasHeader && r == 0) ? headerBg : cellBg;
                sprite.fillRect(x, y, cellW, cellH, bg);
                sprite.drawRect(x, y, cellW, cellH, border);
                sprite.setTextColor(text);
                sprite.setTextFont(2);
                sprite.setTextSize(1);
                int textWidth = sprite.textWidth(cellText[r][c]);
                int textHeight = sprite.fontHeight();
                sprite.setCursor(x + (cellW - textWidth) / 2, y + (cellH - textHeight) / 2);
                sprite.print(cellText[r][c]);
            }
        }
    }

private:
    UIRect bounds;
    uint8_t rows;
    uint8_t cols;
    const char* cellText[MAX_ROWS][MAX_COLS];
    uint16_t headerBg;
    uint16_t cellBg;
    uint16_t border;
    uint16_t text;
};

#endif
