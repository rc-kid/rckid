#pragma once

#include <vector>

#include "../utils/buffers.h"
#include "../utils/timer.h"
#include "../utils/interpolation.h"
#include "../graphics/png.h"
#include "../app.h"
#include "../filesystem.h"
#include "../assets/images.h"
#include "panel.h"
#include "image.h"
#include "header.h"
#include "style.h"


namespace rckid::ui {

    class FormWidget : public Panel {
    public:
        FormWidget():
            FormWidget{320,240} {
        }

        FormWidget(Coord width, Coord height, bool raw = false):
            FormWidget(Rect::Centered(width, height, RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT), raw) {
        }

        FormWidget(Rect rect, bool raw = false): 
            Panel{rect, this},
            buffer_{RCKID_DISPLAY_HEIGHT},
            bgImage_{! raw},
            header_{! raw} {
        }

        
        bool bgImage() const { return bgImage_; }
        bool header() const { return header_; }

        void enableBgImage(bool value = true) {
            if (bgImage_ == value)
                return;
            bgImage_ = value;
        }

        void enableHeader(bool value = true) {
            if (header_ == value)
                return;
            header_ = value;
        }

        static void loadBackgroundImage() {
            if (bg_ != nullptr)
                delete bg_;
            bg_ = new Image{Style::loadBackgroundImage()};
            bg_->setRect(Rect::WH(320, 240));
            //bg_->setRepeat(Style::backgroundScroll() == BackgroundScrollStyle::Scroll);
            bg_->setRepeat(true);
            bg_->setTransparent(false);
        }

        static void clearBackgroundImage() {
            delete bg_;
            bg_ = nullptr;
        }

        void initialize() {
            displaySetRefreshDirection(DisplayRefreshDirection::ColumnFirst);
            displaySetUpdateRegion(rect());
            if (bgImage_ && bg_ == nullptr)
                loadBackgroundImage();
        }

        void finalize() {
            // nothing to do in the finalizer
        }

        /** Renders the form on the display in a column-wise manner.
         */
        void render() {
            // call the draw method of widgets to update drawing parameters before rendering columns
            draw();
            // render columns one by one
            column_ = width() - 1;
            renderColumn(column_, buffer_.front(), 0, height());
            renderColumn(column_ - 1, buffer_.back(), 0, height());
            displayWaitVSync();
            displayUpdate(buffer_.front(), height(), [&](){
                if (--column_ < 0)
                    return;
                buffer_.swap();
                displayUpdate(buffer_.front(), height());
                if (column_ > 0)
                    renderColumn(column_ - 1, buffer_.back(), 0, height());
            });
        }

        static void backgroundTransition(Direction dir, Timer & t) {
            if (bg_ == nullptr)
                return;
            switch (dir) {
                case Direction::Left:
                    bgPos_ = bgStart_ + Point(backgroundTransitionUpdate(t, 80), 0);
                    break;
                case Direction::Right:
                    bgPos_ = bgStart_ - Point(backgroundTransitionUpdate(t, 80), 0);
                    break;
                case Direction::Up:
                    bgPos_ = bgStart_ + Point(0, backgroundTransitionUpdate(t, 60));
                    break;
                case Direction::Down:
                    bgPos_ = bgStart_ - Point(0, backgroundTransitionUpdate(t, 60));
                    break;
                case Direction::None:
                    // don't do anything for no direction
                    break;
                default:
                    UNIMPLEMENTED;
                    break;
            }
            if (! t.running())
                bgStart_ = bgPos_;
        }

        static void refreshStyle() {
            if (bg_ != nullptr) {
                bg_->clear();
                (*bg_) = Style::loadBackgroundImage();
                //bg_->setRepeat(Style::backgroundScroll() == BackgroundScrollStyle::Scroll);
                bgPos_ = Point{0,0};
            }
        }

        /** Returns the currently focused widget, or nullptr if no widget has focus.
         */
        Widget * focused() const { return focused_; }

        /** Focuses given widget. 
         
            The widget should transitively belong to the form. 
         */
        void setFocused(Widget * widget) {
            ASSERT(widget->form() == this);
            focused_ = widget;
        }

        void update() override {
            Panel::update();
            if (focused_ != nullptr)
                focused_->processEvents();
        }

    protected:

        friend class Header;

        void draw() override {
            Panel::draw();
            // if background tilt is enabled, update the actual image size by the current accelerometer values
            if (bg_ != nullptr) {
                if (Style::backgroundTilt())
                    bg_->setImgPos(bgPos_ + Point{accelX() / 512, accelY() / 512});
                else
                    bg_->setImgPos(bgPos_);
            }
        }

        void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
            if (bgImage_) {
                ASSERT(bg_ != nullptr);
                renderChild(bg_, column, buffer, starty, numPixels);
                Widget::renderColumn(column, buffer, starty, numPixels);
            } else {
                Panel::renderColumn(column, buffer, starty, numPixels);    
            }
            if (header_)
                renderChild(Header::instance(), column, buffer, starty, numPixels);
        }

        static Coord backgroundTransitionUpdate(Timer & t, Coord magnitude) {
            switch (Style::backgroundScroll()) {
                case BackgroundScrollStyle::Off:
                    return 0;
                case BackgroundScrollStyle::Scroll:
                    return interpolation::cosine(t, 0, magnitude).round();
                case BackgroundScrollStyle::Nudge:
                    return interpolation::cosineNudge(t, 0, magnitude / 2).round();
                default:
                    UNREACHABLE;
                    return 0;
            }
        }

        DoubleBuffer<uint16_t> buffer_;
        Coord column_;
        bool bgImage_ = true;
        bool header_ = true;

        Widget * focused_ = nullptr;

        static inline Point bgStart_{0,0};
        // position of the background
        static inline Point bgPos_{0,0};
        static inline Image * bg_ = nullptr;
    }; 


    /** App that uses ui::Form as its renderer. 
     
        The class provides basic functionality for all ui based apps. Focus handling saves current focus when the app is blurred and restores when focused.
     */
    template<typename T>
    class Form : public RenderableApp<ui::FormWidget, T> {
    public:
        template <typename... Args>
        Form(Args&&... args) : 
            RenderableApp<ui::FormWidget, T>{std::forward<Args>(args)...} 
        {
        }

    }; 

} // namespace rckid::ui