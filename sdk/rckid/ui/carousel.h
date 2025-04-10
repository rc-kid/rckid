#pragma once

#include "../graphics/bitmap.h"
#include "../utils/interpolation.h"
#include "../utils/timer.h"

#include "image.h"
#include "label.h"
#include "menu.h"

namespace rckid::ui {

    /** The carousel is used to display a rolling menu.
     
        Has two sprites and two element texts, and uses the renderColumn rendering to display them at appropriate parts of the screen. 
     */
    class Carousel : public Widget {
    public:
        enum class Transition {
            None,
            Left,
            Right,
            Up,
            Down,
        };

        static constexpr Coord iconToTextSpacerPx = 5;
        static constexpr uint32_t defaultTransitionTimeMs = 500;

        Carousel() {
            aText_.setHAlign(HAlign::Left);
            aText_.setVAlign(VAlign::Top);
            aText_.setHeight(aText_.font().size);
            bText_.setHAlign(HAlign::Left);
            bText_.setVAlign(VAlign::Top);
            bText_.setHeight(bText_.font().size);
        }

        bool idle() const { return ! a_.running(); }

        Font const & font() { return aText_.font(); }

        void setFont(Font const & f) {
            aText_.setFont(f);
            bText_.setFont(f);
            aText_.setHeight(aText_.font().size);
            bText_.setHeight(bText_.font().size);
            repositionElements(aImg_, aText_);
            repositionElements(bImg_, bText_);
        } 

        void set(Image icon, String text, Transition transition = Transition::None) {
            if (transition == Transition::None) {
                aImg_ = std::move(icon);
                aText_.setText(text);
                repositionElements(aImg_, aText_);
            } else {
                bImg_ = std::move(icon);
                bText_.setText(text);
                repositionElements(bImg_, bText_);
            }
            setTransition(transition);
        }

        void update() override {
            if (dir_ == Transition::None)
                return;
            if (a_.update()) {
                dir_ = Transition::None;
                a_.stop();
                aImgOffset_ = 0;
                aTextOffset_ = 0;
                std::swap(aImg_, bImg_);
                std::swap(aText_, bText_);
            } else {
                updateOffsets();
            }
        }

    protected:

        void set(Menu::Item const & item, Transition transition = Transition::None) {
            if (transition == Transition::None) {
                aImg_ = item.icon();
                aText_.setText(item.text());
                repositionElements(aImg_, aText_);
            } else {
                bImg_ = item.icon();
                bText_.setText(item.text());
                repositionElements(bImg_, bText_);
            }
            setTransition(transition);
        }

        void renderColumn(Coord column, Pixel * buffer, Coord starty, Coord numPixels) override {
            renderChild(& aImg_, column, buffer, starty, numPixels, Point(aImgOffset_, 0));
            renderChild(& aText_, column, buffer, starty, numPixels, Point(aTextOffset_, 0));
            if (dir_ != Transition::None) {
                renderChild(& bImg_, column, buffer, starty, numPixels, Point(bImgOffset_, 0));
                renderChild(& bText_, column, buffer, starty, numPixels, Point(bTextOffset_, 0));
            }
        }

        void repositionElements(Image & imgInto, Label & labelInto) {
            // set the position of the icon and the text label
            Coord tw = labelInto.textWidth();
            labelInto.setWidth(tw);
            Coord x = (width() - (tw + imgInto.width() + iconToTextSpacerPx)) / 2;
            imgInto.setPos(x, (height() - imgInto.height()) / 2);
            x = x + iconToTextSpacerPx + imgInto.width();
            labelInto.setPos(x, (height() - labelInto.font().size) / 2);
        }

        void setTransition(Transition transition) {
            dir_ = transition;
            aImgOffset_ = 0;
            aTextOffset_ = 0;
            if (transition != Transition::None) {
                a_.startContinuous();
                updateOffsets();
            } else {
                a_.stop();
            }
        }

        void updateOffsets() {
            switch (dir_) {
                // new item is coming from left, old goes to right
                case Transition::Left:
                    aImgOffset_ = interpolation::cosine(a_, 0, width()).round();
                    aTextOffset_ = aImgOffset_ * 2;
                    bTextOffset_ = interpolation::cosine(a_, -width(), 0).round();
                    bImgOffset_ = bTextOffset_ * 2;
                    break;
                // new item is coming from right, old goes to left
                case Transition::Right:
                    aTextOffset_ = interpolation::cosine(a_, 0, -width()).round();
                    aImgOffset_ = aTextOffset_ * 2;
                    bImgOffset_ = interpolation::cosine(a_, width(), 0).round();
                    bTextOffset_ = bImgOffset_ * 2;
                    break;
                // new item is coming from sides, old goes down
                case Transition::Up:
                    aTextOffset_ = interpolation::cosine(a_, 0, height()).round();
                    aImgOffset_ = aTextOffset_;
                    bTextOffset_ = interpolation::cosine(a_, width(), 0).round();
                    bImgOffset_ = - bTextOffset_;
                    break;
                // new item is coming the bottom, old goes to the sides
                case Transition::Down:
                    aTextOffset_ = interpolation::cosine(a_, 0, width()).round();
                    aImgOffset_ = - aTextOffset_;
                    bTextOffset_ = interpolation::cosine(a_, height(), 0).round();
                    bImgOffset_ = aTextOffset_;
                    break;
                default:
                    UNREACHABLE;
            }
        }

    private:
        Image aImg_;
        Label aText_;
        Image bImg_;
        Label bText_;

        Transition dir_ = Transition::None;
        Coord aImgOffset_;
        Coord aTextOffset_;
        Coord bImgOffset_;
        Coord bTextOffset_;
        Timer a_{defaultTransitionTimeMs};
    }; // rckid::ui::Carousel


    /** Menu holder that visualizes a carousel. 
     
     */
    class CarouselMenu : public Carousel {
    public:

        CarouselMenu(Menu * m) {
            setMenu(m);
        }

        Menu const * menu() const { return menu_; }

        uint32_t index() const { return i_; }

        void setMenu(Menu * m, Transition transition = Transition::None) {
            Heap::tryFree(menu_);
            menu_ = m;
            if (menu_ == nullptr || menu_->size() == 0)
                return;
            set((*menu_)[0], transition);
        }

        void moveLeft() {
            if (menu_ == nullptr || menu_->size() == 0)
                return;
            i_ = (i_ + menu_->size() - 1) % menu_->size();
            set((*menu_)[i_], Transition::Left);
        }

        void moveRight() {
            if (menu_ == nullptr || menu_->size() == 0)
                return;
            i_ = (i_ + 1) % menu_->size();
            set((*menu_)[i_], Transition::Right);
        }

        /** Processes the left and right menu transitions. 
         */
        void processEvents() override {
            // if there is no menu, don't do anything
            if (menu_ == nullptr || menu_->size() == 0)
                return;
            // if we have ongoing animation, don't do anything
            if (! idle())
                return;
            if (btnDown(Btn::Left))
                moveLeft();
            if (btnDown(Btn::Right))
                moveRight();
        }

    private:

        Menu * menu_ = nullptr;
        uint32_t i_ = 0;

    }; // rckid::ui::CarouselMenu

} // namespace rckid::ui