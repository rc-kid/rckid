#include "canvas.h"

namespace rckid {

    int Canvas::text(char const * str, int x, int y, int size) {
        while (*str != 0)
            x += drawGlyph(*(str++), x, y, fg_, font_, size);
        return x;
    }

    int Canvas::drawGlyph(char c, int x, int y, Color color, GFXFont const * font, int size) {
        GFXGlyph * glyph = font->glyph + (c - font->first);
        uint8_t const * bitmap = font->bitmap + glyph->bitmapOffset;
        int pixelY = y + glyph->yOffset * size;
        int bi = 0;
        uint8_t bits;
        for (int gy = 0; gy < glyph->height; ++gy, pixelY += size) {
            int pixelX = x + glyph->xOffset * size;
            for (int gx = 0; gx < glyph->width; ++gx, pixelX += size, bits <<= 1) {
                if ((bi++ % 8) == 0)
                    bits = *(bitmap++);
                if (bits & 0x80) {
                    //if (size == 1)
                        buffer_[toOffset(pixelX, pixelY)] = color;
                    //else
                    //    fill(Rect::XYWH(pixelX, pixelY, size, size), color);
                }
            }
        }
        return glyph->xAdvance * size;
    }

} // namespace rckid