#pragma once

#include <rckid/ui/widget.h>
#include <rckid/ui/label.h>

namespace rckid::ui {

    class ChatBubble : public Widget {
    public:

        Color fg() const { return fg_; }

        Color bg() const { return bg_; }

        bool isOwn() const { return isOwn_; }

        Coord bubbleWidth() const { return bubbleWidth_ == -1 ? width() - arrowSize_ * 2 : bubbleWidth_; }

        void setFg(Color value) { fg_ = value; }
        void setBg(Color value) { bg_ = value; }
        void setIsOwn(bool value) { isOwn_ = value; }


        /** Renders the chat bubble column. 
         
            Simply draws the shape and then defers to widget's implementation for the contents.
         */
        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            renderBubble(column, startRow, buffer, numPixels);
            Widget::renderColumn(column, startRow, buffer, numPixels);
        }

    protected:

        static constexpr Coord arrowSize_ = 10;

        void renderBubble(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) {
            // first determine the row at which we should switch to background color (when we get inside the bubble), when to switch to foreground color (when we get outside the bubble again) and when to stop rendering.
            Coord switchToBg = 1;
            Coord switchToFg = height() - 1;
            Coord end = height();
            Coord bubbleEndColumn = bubbleWidth() + arrowSize_; 
            // if own message 
            if (isOwn_) {
                bubbleEndColumn = width() - bubbleEndColumn;
                if (column < bubbleEndColumn)
                    return;
                if (column == bubbleEndColumn) {
                    switchToBg = end;
                } else if (column == width() - arrowSize_ - 1) {
                    switchToFg = arrowSize_;
                } else if (column >= width() - arrowSize_) {
                    switchToFg = width() - column - 1;
                    end = switchToFg + 1;
                }
            } else {
                if (column >= bubbleEndColumn)
                    return;
                if (column < arrowSize_) {
                    switchToFg = column;
                    end = switchToFg + 1;
                } else if (column == arrowSize_) {
                    switchToFg = arrowSize_;
                } else if (column == bubbleEndColumn - 1) {
                    switchToBg = end;
                }
            }
            Color::RGB565 color = fg_.toRGB565();
            Coord y = startRow;
            end = std::min(end, startRow + numPixels);
            Color::RGB565 * buf = buffer;
            while (y < end && y < switchToBg) {
                *buf++ = color;
                ++y;
            };
            color = bg_.toRGB565();
            while (y < end && y < switchToFg) {
                *buf++ = color;
                ++y;
            }
            color = fg_.toRGB565();
            while (y < end) {
                *buf++ = color;
                ++y;
            }
        }

        Color fg_;
        Color bg_;

        bool isOwn_ = true; 

        Coord bubbleWidth_ = -1;

    }; // rckid::ui::ChatBubble

    /** Chat bubble containing a multi-line label. 
     */
    class TextChatBubble : public ChatBubble {
    public:

        TextChatBubble() {
            l_.setHAlign(HAlign::Left);
            l_.setVAlign(VAlign::Top);
        }

        String const & text() const { return l_.text(); }
        Color color() const { return l_.color(); }

        void setText(String text) {
            l_.setRect(Rect::XYWH(arrowSize_ + border_, border_, width() - (arrowSize_ + border_) * 2, height() - border_ * 2));
            l_.setHAlign(isOwn() ? HAlign::Right : HAlign::Left);
            l_.setText(std::move(text));
            bubbleWidth_ = l_.textWidth() + border_ * 2;
            setRect(rect().withHeight(l_.textHeight() + border_ * 2));
        }

        void setColor(Color value) { l_.setColor(value); }
        void setColorGradient(Color fg, Color bg) { l_.setColorGradient(fg, bg); }

        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            renderBubble(column, startRow, buffer, numPixels);
            renderChildColumn(&l_, column, startRow, buffer, numPixels);
            Widget::renderColumn(column, startRow, buffer, numPixels);
        }

    protected:

        void onRender() override {
            Widget::onRender();
            triggerOnRender(&l_);
        }

    private:

        static constexpr Coord border_ = 3;

        MultiLabel l_; 
    }; 

} // namespace rckid::ui