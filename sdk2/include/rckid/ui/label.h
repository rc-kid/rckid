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

        HAlign hAlign() const { return textHAlign_; }

        void setHAlign(HAlign value) {
            textHAlign_ = value;
            recalculateHint();
        }

        VAlign vAlign() const { return textVAlign_; }

        void setVAlign(VAlign value) {
            textVAlign_ = value;
            recalculateHint();
        }

        Color color() const { return textColor_; }

        void setColor(Color value) {
            textColor_ = value;
            font_->createFontPalette(textPalette_, textColor_);
        }

        Point textOffset() const { return textOffset_; }

        void setTextOffset(Point value) {
            textOffset_ = value;
            recalculateHint();
        }

        Coord textWidth() const { return textWidth_; }

        void renderColumn(Coord column, Coord starty, Color::RGB565 * buffer, Coord numPixels) override {
            Widget::renderColumn(column, starty, buffer, numPixels);
            adjustRenderParams(textOffset_, column, starty, buffer, numPixels);
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

            /*

            // recalculate starty based on the vertical positioning, do not change starty as we assume it is always relative to the actual text top
            numPixels -= textOffset_.y;
            if (numPixels <= 0)
                return;
            buffer += textOffset_.y;
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
        */
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
                        rightmostHint_ = Hint{textWidth_ + textOffset_.x, gi, text_.size() - 1};
                        break;
                    case HAlign::Center:
                        rightmostHint_ = Hint{(width() - (width() - textWidth_) / 2) + textOffset_.x, gi, text_.size() - 1};
                        break;
                    case HAlign::Right:
                        rightmostHint_ = Hint{width() + textOffset_.x, gi, text_.size() - 1};
                        break;
                    case HAlign::Manual: // do not change
                        break;
                    default:
                        UNREACHABLE;
                }
                switch (textVAlign_) {
                    case VAlign::Top:
                        textOffset_.y = 0;
                        break;
                    case VAlign::Center:
                        textOffset_.y = (height() - font_->size) / 2;
                        break;
                    case VAlign::Bottom:
                        textOffset_.y = height() - font_->size;
                        break;
                    case VAlign::Manual: // do not change
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
        Point textOffset_{0,0};

    }; // ui::Label

    struct SetText {
        String text;
        SetText(String text): text{std::move(text)} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetText st) {
        w->setText(std::move(st.text));
        return w;
    }

    struct SetFont {
        Font font;
        // this cannot be function as we need move-only functions which only exist in C++20 and higher
        SetFont(Font font) : font{std::move(font)} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetFont sf) {
        w->setFont(std::move(sf.font));
        return w;
    }

    struct SetTextOffset {
        Point offset;
        SetTextOffset(Point offset): offset{offset} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetTextOffset sto) {
        w->setTextOffset(sto.offset);
        return w;
    }

} // namespace rckid::ui