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
            Panel{rect},
            buffer_{RCKID_DISPLAY_HEIGHT},
            bgImage_{! raw},
            header_{! raw} {
        }

        
        bool bgImage() const { return bgImage_; }
        bool header() const { return header_; }

        void enableBgImage() {
            if (bgImage_)
                return;
            bgImage_ = true;
        }

        void enableHeader() {
            if (header_)
                return;
            header_ = true;
        }

        void loadBackgroundImage() {
            if (bg_ != nullptr)
                delete bg_;
            auto f = fs::fileRead("files/images/backgrounds/shack16.png");
            if (f.good())
                bg_ = new Image{Bitmap{PNG::fromStream(std::move(f))}};
            else
                bg_ = new Image{Bitmap{PNG::fromBuffer(assets::star)}};
            bg_->setRect(Rect::WH(320, 240));
            bg_->setRepeat(true);
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
                    bg_->setImgX(bgX_ + interpolation::cosine(t, 0, 80).round());
                    break;
                case Direction::Right:
                    bg_->setImgX(bgX_ - interpolation::cosine(t, 0, 80).round());
                    break;
                case Direction::Up:
                    bg_->setImgY(bgY_ + interpolation::cosine(t, 0, 60).round());
                    break;
                case Direction::Down:
                    bg_->setImgY(bgY_ - interpolation::cosine(t, 0, 60).round());
                    break;
                case Direction::None:
                    // don't do anything for no direction
                    break;
                default:
                    UNIMPLEMENTED;
                    break;
            }
            if (! t.running()) {
                bgX_ = bg_->imgX();
                bgY_ = bg_->imgY();
            }
        }

    protected:

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

        DoubleBuffer<uint16_t> buffer_;
        Coord column_;
        bool bgImage_ = true;
        bool header_ = true;

        static inline Coord bgX_ = 0;
        static inline Coord bgY_ = 0;
        static inline Image * bg_ = nullptr;
    }; 


    template<typename T>
    using Form = RenderableApp<ui::FormWidget, T>;

} // namespace rckid::ui