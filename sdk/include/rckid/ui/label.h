#pragma once

#include <rckid/string.h>
#include <rckid/graphics/font.h>
#include <rckid/ui/widget.h>

#include <assets/Iosevka16.h>

namespace rckid::ui {

    class Label : public Widget {
    public:

        String const & text() const { return text_; }

        void setText(String value) {
            text_ = std::move(value);
            onChange();
        }

        /** Takes up to a line from the given string and returns the rest.
         */
        String setTextLine(String const & value) {
            uint32_t end = 0; // end of the text that fits
            uint32_t endSep = 0;
            Coord w = 0;
            while (end < value.size()) {
                char c = value[end];
                GlyphInfo const * gi = font_->glyphInfoFor(c);
                ASSERT(gi != nullptr);
                if (w + gi->advanceX > width())
                    break;
                w += gi->advanceX;
                ++end;
                if (String::isWordSeparator(c))
                    endSep = end;
                if (c == '\n')
                    break;
            }
            uint32_t start; // start of the remainder
            if ((endSep > 0) && (end < value.size() - 1)) {
                end = endSep - 1;
                start = endSep;
            } else {
                start = end;
            }
            String res = value.substr(start);
            String newText = value.substr(0, end);
            setText(std::move(newText));
            return res;
        }

        Font const & font() const { return font_; }

        void setFont(Font value) {
            ASSERT(value != nullptr);
            font_ = std::move(value);
            onChange();
        }

        HAlign hAlign() const { return textHAlign_; }

        void setHAlign(HAlign value) {
            textHAlign_ = value;
            onChange();
        }

        VAlign vAlign() const { return textVAlign_; }

        void setVAlign(VAlign value) {
            textVAlign_ = value;
            onChange();
        }

        Color fg() const { return textColor_; }

        void setFg(Color value, Color bg = Color::Black()) {
            textColor_ = value;
            font_->createFontPalette(textPalette_, bg, textColor_);
        }

        /** Sets the color gradient. 
         
            Specifies the 4 color levels as a gradient between the foreground (font color), and background color. 

            TODO remove this and use the setColor method above instead
         */
        void setFgGradient(Color fg, Color bg) {
            textColor_ = fg;
            font_->createFontPalette(textPalette_, bg, textColor_);
        }

        /** Alpha rendering for the label means that the font pixels will not be rendered using static predefined palette, but will blend the desired font color and the existinfg pixel in the rendering buffer according to the font specification. 
         
            This means the rendering itself takes longer, but in cases where the rendering happens over surfaces with inconsistent background colors leads to much greater visibility.
         */
        bool useAlpha() const { return useAlpha_; }

        void setUseAlpha(bool value) {
            useAlpha_ = value;
        }

        Point textOffset() const { return textOffset_; }

        void setTextOffset(Point value) {
            textOffset_ = value;
            onChange();
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
            if (!useAlpha_)
                font_->renderColumn(column, starty, numPixels, currentHint_.gi, buffer, textPalette_);
            else 
                font_->renderColumnAlpha(column, starty, numPixels, currentHint_.gi, buffer, textColor_);
        }

        void applyStyle(Style const & style) override {
            Widget::applyStyle(style);
            setFg(style.defaultFg());
        }

        protected:
        void onRender() override {
            // reset the rendering hint to the rightmost character
            currentHint_ = rightmostHint_;
            Widget::onRender();
        }

    private:

        friend class MultiLabel;

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

        void onChange() override {
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
        // whether to use slower per pixel blending when rendering the characters
        bool useAlpha_ = false;
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

    struct SetUseAlpha {
        bool value;
        SetUseAlpha(bool value): value{value} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetUseAlpha x) {
        w->setUseAlpha(x.value);
        return w;
    }

    /** Multi-line label.
     
        A simple aggregate class that is merely a container for multiple labels, each label being a single line of text. This allows us a simple way of rendering multi-line text without the need of specialized drawing pipelines.
     */
    class MultiLabel : public Widget {
    public:

        String const & text() const { return text_; }

        void setText(String text) {
            text_ = std::move(text);
            recalculateLines();
        }

        Font const & font() const { return font_; }

        void setFont(Font font) {
            font_ = std::move(font);
            recalculateLines();
        }

        HAlign hAlign() const { return textHAlign_; }

        void setHAlign(HAlign value) {
            textHAlign_ = value;
            for (auto & line : lines_)
                line->setHAlign(value);
        }

        VAlign vAlign() const { return textVAlign_; }

        void setVAlign(VAlign value) {
            textVAlign_ = value;
            repositionLines();
        }

        Color fg() const { return textColor_; }

        void setFg(Color value) {
            textColor_ = value;
            for (auto & line : lines_)
                line->setFg(value);
        }

        void setFgGradient(Color fg, Color bg) {
            textColor_ = fg;
            for (auto & line : lines_)
                line->setFgGradient(fg, bg);
        }

        Coord textWidth() const { 
            Coord result = 0;
            for (auto & line : lines_) {
                Coord w = line->textWidth();
                if (w > result)
                    result = w;
            }
            return result;
        }

        Coord textHeight() const { return font_->size * lines_.size(); }

        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            ASSERT(verifyRenderParams(width(), height(), column, startRow, numPixels));
            for (auto & line : lines_)
                renderChildColumn(line.get(), column, startRow, buffer, numPixels);
            Widget::renderColumn(column, startRow, buffer, numPixels);
        }

    protected:
        void onChange() override {
            Widget::onChange();
            recalculateLines();
        }

        void onRender() override {
            // reset the rendering hint to the rightmost character
            for (auto & line : lines_)
                 line->onRender();
            Widget::onRender();
        }

        void recalculateLines() {
            lines_.clear();
            String txt = text_;
            while (!txt.empty()) {
                unique_ptr<Label> line = std::make_unique<Label>();
                with(line)
                    << SetFont{font_}
                    << SetHAlign{textHAlign_}
                    << SetVAlign{VAlign::Top}
                    << SetFg{textColor_}
                    << SetRect(Rect::WH(width(), font_->size));
                txt = line->setTextLine(txt);
                lines_.push_back(std::move(line));
            }
            repositionLines();
        }

        void repositionLines() {
            if (lines_.empty())
                return;
            Coord h = font_->size * lines_.size();
            Coord y;
            switch (textVAlign_) {
                case VAlign::Top:
                    y = 0;
                    break;
                case VAlign::Center:
                    y = (height() - h) / 2;
                    break;
                case VAlign::Bottom:
                    y = height() - h;
                    break;
                case VAlign::Manual:
                    y = lines_[0]->y();
                    break;
                default:
                    UNREACHABLE;
            }
            for (auto & line : lines_) {
                Rect r = line->rect();
                r.y = y;
                line->setRect(r);
                y += font_->size;
            }
        }

    private:
        String text_;
        Font font_{assets::Iosevka16};
        HAlign textHAlign_ = HAlign::Left;
        VAlign textVAlign_ = VAlign::Center;
        Color textColor_ = Color::White();

        std::vector<unique_ptr<Label>> lines_;

    }; // ui::MultiLabel

} // namespace rckid::ui