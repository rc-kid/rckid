#pragma once

#include <functional>

#include "../graphics/bitmap.h"
#include "../graphics/icon.h"
#include "../utils/interpolation.h"
#include "../utils/timer.h"
#include "../assets/icons_64.h"

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

        /** Sets the carousel's current element to given text and icon. 
         */
        void set(String text, Icon const & icon, Direction direction = Direction::None) {
            if (direction == Direction::None) {
                icon.intoBitmap(aImg_.bitmap());
                aImg_.shrinkToFit();
                aImg_.setTransparent(true);
                aText_.setText(text);
                repositionElements(aImg_, aText_);
            } else {
                icon.intoBitmap(bImg_.bitmap());
                bImg_.shrinkToFit();
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
                dir_ = Direction::None;
                aImgOffset_ = 0;
                aTextOffset_ = 0;
                std::swap(aImg_, bImg_);
                std::swap(aText_, bText_);
            } else {
                Form::backgroundTransition(initialized_ >= 2 ? dir_ : Direction::None, a_);
                updateOffsets();
            }
        }

        void clear() {
            aImg_.clear();
            aText_.clear();
            bImg_.clear();
            bText_.clear();
        }

        void showEmpty(Direction d = Direction::None) {
            set("", Icon{assets::icons_64::empty_box}, d);
        }

        uint32_t currentIndex() const { return i_; }

        /** Returns the number of elements in the carousel. 
         */
        virtual uint32_t size() const = 0;

        void moveLeft() {
            uint32_t s = size();
            if (s == 0)
                return;
            i_ = (i_ == 0) ? (s - 1) : (i_ - 1);
            setItem(i_, Direction::Left);
        }

        void moveRight() {
            uint32_t s = size();
            if (s == 0)
                return;
            ++i_;
            if (i_ >= s)
                i_ = 0;
            setItem(i_, Direction::Right);
        }

        void processEvents() override {
            if (!idle() || size() == 0)
                return;
            if (btnDown(Btn::Left))
                moveLeft();
            if (btnDown(Btn::Right))
                moveRight();
        }

        void setItem(uint32_t index, Direction direction = Direction::None) {
            i_ = index;
            doSetItem(index, direction);
        }

    protected:

        virtual void doSetItem(uint32_t index, Direction direction) = 0;

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

        uint32_t i_ = 0;
    }; // rckid::ui::Carousel

    /** Carousel specialization with event handlers instead of the virtual abstract function for size & element setting. Using this class is beneficial for custom carousels where it allows easier customization compared to subclassing carousel and overriding the methods.
     */
    class EventBasedCarousel : public Carousel {
    public:
        EventBasedCarousel(std::function<uint32_t()> getSizeHandler, std::function<void(uint32_t, Direction)> setItemHandler):
            getSizeHandler_{std::move(getSizeHandler)},
            setItemHandler_{std::move(setItemHandler)} {
        }

        uint32_t size() const override {
            return getSizeHandler_();
        }

    protected:

        void doSetItem(uint32_t index, Direction direction) override {
            setItemHandler_(index, direction);
        }        

    private:
        std::function<uint32_t()> getSizeHandler_;
        std::function<void(uint32_t, Direction)> setItemHandler_;
    };

    class CarouselMenu : public Carousel {
    public:

        ~CarouselMenu() override {
            delete menu_;
        }

        Menu const * menu() const { return menu_; }

        uint32_t size() const override { return (menu_ == nullptr) ? 0 : menu_->size(); }

        void setMenu(Menu * m) {
            delete menu_;
            menu_ = m;
        }

        Menu::Item * currentItem() {
            if (menu_ == nullptr || menu_->size() == 0)
                return nullptr;
            return & (*menu_)[currentIndex()];
        }


    protected:

        void doSetItem(uint32_t index, Direction direction = Direction::None) override {
            ASSERT(menu_ != nullptr);
            ASSERT(index < menu_->size());
            Menu::Item const & item = (*menu_)[index];
            set(item.text, item.icon, direction);
        }

    private:

        Menu * menu_ = nullptr;
    }; 

} // namespace rckid::ui