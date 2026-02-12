#pragma once

#include <rckid/ui/widget.h>
#include <rckid/ui/animation.h>


namespace rckid::ui {

    /** Wraps anything with  width & height & renderColumn into a widget.
     
        A simple little class that takes something that is renderable, i.e. what has width(), height() and renderColumn() methods and wraps it into a widget. The wrapper class also support alignment of its contents in the widget itself as well as its optional repeating. 

        This is iseful for many renderable, but non-widget elements, such as bitmaps, tilemaps, sprites, etc.
     */
    template<typename T>
    class Wrapper : public Widget {
    public:

        Wrapper() = default;
        Wrapper(T contents): contents_{std::move(contents)} {}

        T const & contents() const { return contents_; }

        void setContents(T contents) {
            contents_ = std::move(contents);
            onResize();
        }

        HAlign hAlign() const { return contentsHAlign_; }

        void setHAlign(HAlign value) {
            contentsHAlign_ = value;
            onResize();
        }

        VAlign vAlign() const { return contentsVAlign_; }

        void setVAlign(VAlign value) {
            contentsVAlign_ = value;
            onResize();
        }

        bool contentsRepeat() const { return contentsRepeat_; }

        void setContentsRepeat(bool value) {
            contentsRepeat_ = value;
        }

        Point contentsOffset() const { return contentsOffset_; }

        void setContentsOffset(Point value) {
            contentsOffset_ = value;
            onResize();
        }

        void renderColumn(Coord column, Coord starty, Color::RGB565 * buffer, Coord numPixels) override {
            Widget::renderColumn(column, starty, buffer, numPixels);
            // if we are no repeating the contents, we must adjust the rendering parameters based on the contents position and size for the shared rendering to work
            if (! contentsRepeat_) {
                // adjust the column & starty based on the contents position
                adjustRenderParams(contentsOffset_, column, starty, buffer, numPixels);
                // check if we are outside of the contents
                if (column < 0 || column >= contents_.width() || starty >= contents_.height())
                    return;
                // adjust numPixels if we exceed contents height
                if (starty + numPixels > contents_.height())
                    numPixels = contents_.height() - starty;
                if (numPixels <= 0)
                    return;
            } else {
                // adjust the column to be a valid contents column
                column = (column - contentsOffset_.x) % contents_.width();
                if (column < 0)
                    column += contents_.width();
                // similiarly adjust startx to corresponding contents row
                starty = (starty - contentsOffset_.y) % contents_.height();
                if (starty < 0)                    
                    starty += contents_.height();
            }
            // render as many pixels as we have to by repeating the image (numPixels were updated accordingly if no repeat is required)
            while (numPixels > 0) {
                Coord n = std::min(numPixels, contents_.height() - starty);
                contents_.renderColumn(column, starty, n, buffer);
                numPixels -= n;
                buffer += n;
                starty = 0; // after the first iteration, we will always start at the
            }
        }

    protected:
        void onResize() override {
            Widget::onResize();
            Coord x = contentsOffset_.x;
            Coord y = contentsOffset_.y;
            switch (contentsHAlign_) {
                case HAlign::Left:
                    x = 0;
                    break;
                case HAlign::Center:
                    x = (width() - contents_.width()) / 2;
                    break;
                case HAlign::Right:
                    x = width() - contents_.width();
                    break;
                case HAlign::Manual: // do not change
                    break;
                default:
                    UNREACHABLE;
            }
            switch (contentsVAlign_) {
                case VAlign::Top:
                    y = 0;
                    break;
                case VAlign::Center:
                    y = (height() - contents_.height()) / 2;
                    break;
                case VAlign::Bottom:
                    y = height() - contents_.height();
                    break;
                case VAlign::Manual: // do not change
                    break;
                default:
                    UNREACHABLE;
            }
            contentsOffset_ = Point{x, y};
        }

        T contents_;
        HAlign contentsHAlign_ = HAlign::Center;
        VAlign contentsVAlign_ = VAlign::Center;
        Point contentsOffset_ = Point{0,0};
        bool contentsRepeat_ = false;

    }; // rckid::ui::Wrapper<T>

    template<typename T>
    struct SetContents {
        T contents;
        SetContents(T contents): contents{std::move(contents)} {}
    };
    template<typename T>
    inline with<Wrapper<T>> operator << (with<Wrapper<T>> w, SetContents<T> sc) {
        w->setContents(std::move(sc.contents));
        return w;
    }

    struct SetContentsRepeat {
        bool repeat;
        SetContentsRepeat(bool repeat): repeat{repeat} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetContentsRepeat scr) {
        w->setContentsRepeat(scr.repeat);
        return w;
    }

    struct SetContentsOffset {
        Point offset;
        SetContentsOffset(Point offset): offset{offset} {}
        SetContentsOffset(Coord x, Coord y): offset{x,y} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetContentsOffset sco) {
        w->setContentsOffset(sco.offset);
        return w;
    }

    template<typename T>
    inline Animation * OffsetContents(Wrapper<T> * target, Point from, Point to, uint32_t durationMs) {
        return (new Animation{
            [from, to, target](FixedRatio progress) {
                Coord x = from.x + progress.scale(to.x - from.x);
                Coord y = from.y + progress.scale(to.y - from.y);
                target->setContentsOffset(Point{x, y});
            },
            durationMs
        })->setEasingFunction(easing::inOut);
    }

} // namespace rckid::ui