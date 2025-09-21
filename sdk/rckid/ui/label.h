#pragma once

#include "../assets/fonts/Iosevka16.h"
#include "../graphics/font.h"
#include "../graphics/color.h"
#include "../utils/string.h"
#include "widget.h"

namespace rckid::ui {

    /** Text label
     */
    class Label : public Widget {
    public:

        Label() = default;

        Label(Coord x, Coord y, String text):
            Widget{x, y}, 
            text_{std::move(text)} {
            if (autosize_)
                resizeToText();
            else
                reposition();
        }

        Label(Point pos, String text): Label{pos.x, pos.y, std::move(text)} {}

        Label(Rect rect, String text):
            Widget{rect}, 
            text_{std::move(text)},
            autosize_{false} {
            reposition();
        }

        String const & text() { return text_; }

        bool autosize() const { return autosize_; }
        void setAutosize(bool value) { 
            if (autosize_ != value) {
                autosize_ = value;
                if (autosize_)
                    resizeToText();
            }
        }

        /** Sets the text of the label and returns true if the text differs from previous value, false otherwise. Setting text that is already in the label is a no-op.
         */
        bool setText(String value) { 
            if (value == text_)
                return false;
            text_ = std::move(value);
            if (autosize_)
                resizeToText();
            else
                reposition();
            return true;
        }

        void resizeToText() {
            w_ = textWidth();
            h_ = font_.size;
            reposition();
        }

        ColorRGB color() const { return color_; }
        void setColor(ColorRGB value) { color_ = value; }

        Font const & font() const { return font_; }
        void setFont(Font const & value) { 
            font_ = value;
            if (autosize_)
                resizeToText();
            else
                reposition();
        }

        HAlign hAlign() const { return hAlign_; }
        VAlign vAlign() const { return vAlign_; }

        void setHAlign(HAlign value) {
            if (value != hAlign_) {
                hAlign_ = value;
                reposition();
            }
            return;
        }

        void setVAlign(VAlign value) {
            if (value != vAlign_) {
                vAlign_ = value;
                reposition();
            }
        }

        Coord textWidth() const { return font_.textWidth(text_.c_str()); }

        void clear() {
            setText("");
        }

    protected:

        void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
            // don't do anything we are vertically off 
            if (starty >= textTopLeft_.y + font_.size)
                return;
            if (starty + numPixels < textTopLeft_.y)
                return;
            // this means figuring out which character to print and then 
            for (Hint const & h : hints_) {
                // nothing to render if the column is *after* the current glyph to draw
                if (column >= h.right)
                    return;
                // if the column is less than the beginning of the glyph, move to next one
                if (column < h.left)
                    continue;
                // otherwise we will be drawing the current glyph, so update the rendering structures to its rectangle
                adjustRenderParams(Rect::XYWH(h.left, textTopLeft_.y + h.gi->y, h.gi->width, h.gi->height), column, buffer, starty, numPixels);
                if (numPixels == 0)
                    return;
                ASSERT(column >= 0 && column < h.gi->width);
                ASSERT(numPixels <= h.gi->height);
                uint16_t colors[] = {
                    0, 
                    color_.withAlpha(85),
                    color_.withAlpha(170),
                    color_
                };
                font_.renderColumn(column, starty, numPixels, h.gi, buffer, colors);
                return;
            }
        }

        void resize() override {
            reposition();
        }

        void reposition() {
            hints_.clear();
            Coord tw = font_.textWidth(text_.c_str());
            textTopLeft_ = justifyRectangle(Rect::WH(tw, font_.size), hAlign_, vAlign_);
            Coord x = textTopLeft_.x;
            for (char c : text_) {
                GlyphInfo const * gi = & font_.glyphInfoFor(c);
                hints_.push_back(Hint{x, gi});
                x += gi->advanceX;
            }
            std::reverse(hints_.begin(), hints_.end());
        }

    private:
        struct Hint {
            Coord left;
            Coord right;
            GlyphInfo const * gi;

            Hint(Coord x, GlyphInfo const * gi):
                left{x + gi->x}, 
                right{x + gi->x + gi->width}, 
                gi{gi} {
            }
        };

        HAlign hAlign_ = HAlign::Center;
        VAlign vAlign_ = VAlign::Center;
        String text_;
        Font font_{Font::fromROM<assets::Iosevka16>()};
        ColorRGB color_{ColorRGB::White()};
        Point textTopLeft_;
        std::vector<Hint> hints_;
        bool autosize_ = true;
    }; //rckid::ui::Label

} // namespace rckid::ui