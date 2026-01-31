#pragma once

#include <rckid/string.h>
#include <rckid/graphics/font.h>
#include <rckid/ui/widget.h>

#include <assets/Iosevka16.h>

namespace rckid::ui {

    class Label : public Widget {
    public:

        void setRect(Rect rect) override {
            Widget::setRect(rect);
            recalculateHint();
        }

        String const & text() const { return text_; }

        void setText(String value) {
            text_ = std::move(value);
            recalculateHint();
        }

        Font const & font() const { return font_; }

        void setFont(Font value) {
            ASSERT(value != nullptr);
            font_ = std::move(value);
            recalculateHint();
        }

        HAlign textHAlign() const { return textHAlign_; }

        void setTextHAlign(HAlign value) {
            textHAlign_ = value;
            recalculateHint();
        }

        VAlign textVAlign() const { return textVAlign_; }

        void setTextVAlign(VAlign value) {
            textVAlign_ = value;
            recalculateHint();
        }

        Color textColor() const { return textColor_; }

        void setTextColor(Color value) {
            textColor_ = value;
            font_->createFontPalette(textPalette_, textColor_);
        }

        void renderColumn(Coord column, Coord starty, Color::RGB565 * buffer, Coord numPixels) override {
            Widget::renderColumn(column, starty, buffer, numPixels);
            // recalculate starty based on the vertical positioning, do not change starty as we assume it is always relative to the actual text top
            numPixels -= rowOffset_;
            if (numPixels <= 0)
                return;
            buffer += rowOffset_;
            // move to previous hint if needed
            if (column < currentHint_.leftColumn) {
                if (currentHint_.charOffset == 0) // no more characters left
                    return;
                GlyphInfo const * gi = font_->glyphInfoFor(text_[currentHint_.charOffset - 1]);
                currentHint_ = Hint{currentHint_.leftColumn, gi, currentHint_.charOffset - 1};
            }
            // don't render if we are too far
            if (column >= currentHint_.rightColumn)
                return;
            // recalculate column & starty relative to the text position
            column -= currentHint_.leftColumn;
            font_->renderColumn(column, starty, numPixels, currentHint_.gi, buffer, textPalette_);
        }

    protected:
        void onRender() override {
            // reset the rendering hint to the rightmost character
            currentHint_ = rightmostHint_;
            Widget::onRender();
        }

    private:

        struct Hint {
            Coord leftColumn = -1;
            Coord rightColumn = -1;
            GlyphInfo const * gi = nullptr;
            uint32_t charOffset = 0;

            Hint(Coord rightColumn, GlyphInfo const * gi, uint32_t charOffset):
                leftColumn{rightColumn - gi->advanceX}, 
                rightColumn{rightColumn}, 
                gi{gi},
                charOffset{charOffset} {
            }

            Hint() = default;
        };

        void recalculateHint() {
            if (text_.empty()) {
                textWidth_ = 0;
                rightmostHint_ = Hint{};
            } else {
                textWidth_ = font_->textWidth(text_);
                GlyphInfo const * gi = font_->glyphInfoFor(text_[text_.size() - 1]);
                switch (textHAlign_) {
                    case HAlign::Left:
                        rightmostHint_ = Hint{textWidth_, gi, text_.size() - 1};
                        break;
                    case HAlign::Center:
                        rightmostHint_ = Hint{(width() - (width() - textWidth_) / 2), gi, text_.size() - 1};
                        break;
                    case HAlign::Right:
                        rightmostHint_ = Hint{width(), gi, text_.size() - 1};
                        break;
                    default:
                        UNREACHABLE;
                }
                switch (textVAlign_) {
                    case VAlign::Top:
                        rowOffset_ = 0;
                        break;
                    case VAlign::Center:
                        rowOffset_ = (height() - font_->size) / 2;
                        break;
                    case VAlign::Bottom:
                        rowOffset_ = height() - font_->size;
                        break;
                    default:
                        UNREACHABLE;
                }
            }
        }

        String text_; 
        Coord textWidth_ = 0;
        Font font_{assets::Iosevka16};
        HAlign textHAlign_ = HAlign::Left;
        VAlign textVAlign_ = VAlign::Center;
        Color textColor_ = Color::White();
        Color::RGB565 textPalette_[4] = {
            0, 
            Color::White().withBrightness(85).toRGB565(), 
            Color::White().withBrightness(170).toRGB565(),
            Color::White().toRGB565()
        };
        // current character rendering hint
        Hint currentHint_;
        // this is where we store the rightmost character hint (the first one to be rendered due to right to left top to bottom rendering)
        Hint rightmostHint_;
        Coord rowOffset_ = 0;

    }; // ui::Label

    inline auto SetText(String text) {
        return [t = std::move(text)](Label * l) { l->setText(std::move(t)); };
    }

    inline auto SetTextHAlign(HAlign align) {
        return [=](Label * l) { l->setTextHAlign(align); };
    }

    inline auto SetTextVAlign(VAlign align) {
        return [=](Label * l) { l->setTextVAlign(align); };
    }

    struct SetFont {
        // this cannot be function as we need move-only functions which only exist in C++20 and higher
        SetFont(Font font) : font_{std::move(font)} {}

        void operator () (Label * l) const {
            l->setFont(std::move(font_));
        }
    private:        
        mutable Font font_;
    };

} // namespace rckid::ui