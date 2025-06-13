#pragma once

#include <functional>

#include "../graphics/bitmap.h"
#include "../utils/interpolation.h"
#include "../utils/timer.h"

#include "image.h"
#include "label.h"
#include "menu.h"
#include "form.h"

namespace rckid::ui {

    /** The carousel is used to display a rolling menu.
     
        Has two sprites and two element texts, and uses the renderColumn rendering to display them at appropriate parts of the screen. The carousel only concerns itself with rendering of the current item and directions between items. Its purpose is to be the smallest reusable block. For better UI experience, use the CarouselMenu widget which incorporates extra features such as menu management, etc. 
     */
    class Carousel : public Widget {
    public:

        enum class TransitionState {
            InProgress,
            Start,
            End,
        };

        using OnTransition = std::function<void(TransitionState, Direction, Timer & )>;

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

        void set(String text, Image icon, Direction direction = Direction::None) {
            if (direction == Direction::None) {
                aImg_ = std::move(icon);
                aImg_.setTransparent(true);
                aText_.setText(text);
                repositionElements(aImg_, aText_);
            } else {
                bImg_ = std::move(icon);
                bImg_.setTransparent(true);
                bText_.setText(text);
                repositionElements(bImg_, bText_);
            }
            if (initialized_ < 2)
                initialized_++;
            setTransition(direction);
        }

        void update() override {
            if (dir_ == Direction::None)
                return;
            if (a_.update()) {
                a_.stop();
                Form::backgroundTransition(initialized_ >= 2 ? dir_ : Direction::None, a_);
                if (onTransition_)
                    onTransition_(TransitionState::End, dir_, a_);
                dir_ = Direction::None;
                aImgOffset_ = 0;
                aTextOffset_ = 0;
                std::swap(aImg_, bImg_);
                std::swap(aText_, bText_);
            } else {
                Form::backgroundTransition(initialized_ >= 2 ? dir_ : Direction::None, a_);
                if (onTransition_)
                    onTransition_(TransitionState::InProgress, dir_, a_);
                updateOffsets();
            }
        }

        void setOnTransitionEvent(OnTransition e) {
            onTransition_ = e;
        }

        void clear() {
            aImg_.clear();
            aText_.clear();
            bImg_.clear();
            bText_.clear();
        }

    protected:

        void set(Menu::Item const & item, Direction direction = Direction::None) {
            set(item.text(), item.icon(), direction);
        }

        void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
            switch (dir_) {
                case Direction::Left:
                case Direction::Right:
                    renderChild(& bImg_, column, buffer, starty, numPixels, Point(bImgOffset_, 0));
                    renderChild(& bText_, column, buffer, starty, numPixels, Point(bTextOffset_, 0));
                    [[fallthrough]];
                case Direction::None:
                    renderChild(& aImg_, column, buffer, starty, numPixels, Point(aImgOffset_, 0));
                    renderChild(& aText_, column, buffer, starty, numPixels, Point(aTextOffset_, 0));
                    break;                      
                case Direction::Up:
                    renderChild(& aImg_, column, buffer, starty, numPixels, Point(0, aImgOffset_));
                    renderChild(& aText_, column, buffer, starty, numPixels, Point(0, aTextOffset_));
                    renderChild(& bImg_, column, buffer, starty, numPixels, Point(bImgOffset_, 0));
                    renderChild(& bText_, column, buffer, starty, numPixels, Point(bTextOffset_, 0));
                    break;
                case Direction::Down:
                    renderChild(& aImg_, column, buffer, starty, numPixels, Point(aImgOffset_, 0));
                    renderChild(& aText_, column, buffer, starty, numPixels, Point(aTextOffset_, 0));
                    renderChild(& bImg_, column, buffer, starty, numPixels, Point(0, bImgOffset_));
                    renderChild(& bText_, column, buffer, starty, numPixels, Point(0, bTextOffset_));
                    break;
                default:
                    UNREACHABLE;
                    break;
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

        void setTransition(Direction direction) {
            dir_ = direction;
            aImgOffset_ = 0;
            aTextOffset_ = 0;
            if (direction != Direction::None) {
                a_.startContinuous();
                updateOffsets();
                Form::backgroundTransition(initialized_ >= 2 ? dir_ : Direction::None, a_);
                if (onTransition_)
                    onTransition_(TransitionState::Start, dir_, a_);
            } else {
                a_.stop();
            }
        }

        void updateOffsets() {
            switch (dir_) {
                // new item is coming from left, old goes to right
                case Direction::Left:
                    aImgOffset_ = interpolation::cosine(a_, 0, width()).round();
                    aTextOffset_ = aImgOffset_ * 2;
                    bTextOffset_ = interpolation::cosine(a_, -width(), 0).round();
                    bImgOffset_ = bTextOffset_ * 2;
                    break;
                // new item is coming from right, old goes to left
                case Direction::Right:
                    aTextOffset_ = interpolation::cosine(a_, 0, -width()).round();
                    aImgOffset_ = aTextOffset_ * 2;
                    bImgOffset_ = interpolation::cosine(a_, width(), 0).round();
                    bTextOffset_ = bImgOffset_ * 2;
                    break;
                // new item is coming from sides, old goes down
                case Direction::Up:
                    aTextOffset_ = interpolation::cosine(a_, 0, height()).round();
                    aImgOffset_ = aTextOffset_;
                    bTextOffset_ = interpolation::cosine(a_, width(), 0).round();
                    bImgOffset_ = - bTextOffset_;
                    break;
                // new item is coming from the bottom, old goes to the sides
                case Direction::Down:
                    aTextOffset_ = interpolation::cosine(a_, 0, width()).round();
                    aImgOffset_ = - aTextOffset_;
                    bTextOffset_ = interpolation::cosine(a_, height(), 0).round();
                    bImgOffset_ = bTextOffset_;
                    break;
                default:
                    UNREACHABLE;
            }
        }

    private:
        uint32_t initialized_ = 0;
        Image aImg_;
        Label aText_;
        Image bImg_;
        Label bText_;

        Direction dir_ = Direction::None;
        Coord aImgOffset_;
        Coord aTextOffset_;
        Coord bImgOffset_;
        Coord bTextOffset_;
        Timer a_{defaultTransitionTimeMs};

        OnTransition onTransition_;
    }; // rckid::ui::Carousel


    /** Menu holder that visualizes a carousel. 
     
     */
    class CarouselMenu : public Carousel {
    public:

        CarouselMenu() = default;

        ~CarouselMenu() override {
            delete menu_;
        }

        Menu const * menu() const { return menu_; }

        uint32_t index() const { return i_; }

        Menu::Item * currentItem() {
            if (menu_ == nullptr || menu_->size() == 0)
                return nullptr;
            return & (*menu_)[i_];
        }

        void setMenu(Menu * m, Direction direction = Direction::None, uint32_t index = 0) {
            delete menu_;
            menu_ = m;
            if (menu_ == nullptr || menu_->size() == 0)
                return;
            set((*menu_)[index], direction);
            i_ = index;
        }

        void moveLeft() {
            if (menu_ == nullptr || menu_->size() == 0)
                return;
            i_ = (i_ + menu_->size() - 1) % menu_->size();
            set((*menu_)[i_], Direction::Left);
        }

        void moveRight() {
            if (menu_ == nullptr || menu_->size() == 0)
                return;
            i_ = (i_ + 1) % menu_->size();
            set((*menu_)[i_], Direction::Right);
        }

        /** Processes the left and right menu directions. 
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