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
            // whenever we call set, we are not empty (if called via showEmpty, we'll set empty later)
            empty_ = false;
            if (direction == Direction::None) {
                aImg_ = icon;
                aImg_.setTransparent(true);
                aText_.setText(text);
                aText_.setColor(ui::Style::fg());
                repositionElements(aImg_, aText_);
            } else {
                bImg_ = icon;
                bImg_.setTransparent(true);
                bText_.setText(text);
                bText_.setColor(ui::Style::fg());
                repositionElements(bImg_, bText_);
            }
            if (initialized_ < 2)
                initialized_++;
            setTransition(direction);
        }

        void draw() override {
            if (dir_ == Direction::None)
                return;
            if (a_.update()) {
                a_.stop();
                FormWidget::backgroundTransition(initialized_ >= 2 ? dir_ : Direction::None, a_);
                dir_ = Direction::None;
                aImgOffset_ = 0;
                aTextOffset_ = 0;
                std::swap(aImg_, bImg_);
                std::swap(aText_, bText_);
            } else {
                FormWidget::backgroundTransition(initialized_ >= 2 ? dir_ : Direction::None, a_);
                updateOffsets();
            }
            Widget::draw();
        }

        void clear() {
            aImg_.clear();
            aText_.clear();
            bImg_.clear();
            bText_.clear();
        }

        void showEmpty(Direction d = Direction::None) {
            set("Empty", Icon{assets::icons_64::empty_box}, d);
            bText_.setColor(ColorRGB::RGB(64, 64, 64));
            empty_ = true;
        }

        bool empty() const { return empty_; }

        uint32_t currentIndex() const { return i_; }

        /** Returns the number of elements in the carousel. 
         */
        virtual uint32_t size() const = 0;

        uint32_t getIndexLeft() const {
            uint32_t s = size();
            if (s == 0)
                return 0;
            return (i_ == 0) ? (s - 1) : (i_ - 1);
        }

        uint32_t getIndexRight() const {
            uint32_t s = size();
            if (s == 0)
                return 0;
            return (i_ + 1) % s;
        }

        void moveLeft() {
            setItem(getIndexLeft(), Direction::Left);
        }

        void moveRight() {
            setItem(getIndexRight(), Direction::Right);
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

        Point iconPosition() const { return aImg_.pos() + pos(); }
        Point textPosition() const { return aText_.pos() + pos(); }

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
                FormWidget::backgroundTransition(initialized_ >= 2 ? dir_ : Direction::None, a_);
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
        bool empty_ = false;
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

    template<typename PAYLOAD>
    class CarouselMenu : public Carousel {
    public:

        ~CarouselMenu() override {
            delete menu_;
        }

        Menu<PAYLOAD> const * menu() const { return menu_; }

        uint32_t size() const override { return (menu_ == nullptr) ? 0 : menu_->size(); }

        void setMenu(typename Menu<PAYLOAD>::MenuGenerator generator) {
            generator_ = generator;
            delete menu_;
            menu_ = generator_();
            setItem(0, Direction::Up);
        }

        typename Menu<PAYLOAD>::MenuItem * currentItem() {
            if (menu_ == nullptr || menu_->size() == 0)
                return nullptr;
            return & (*menu_)[currentIndex()];
        }

        void processEvents() override {
            if (!idle()) // do not accept extra events if idle
                return;
            if (btnPressed(Btn::Up) || btnPressed(Btn::A)) {
                auto item = currentItem();
                if (item == nullptr)
                    return; // nothing to process 
                if (item->isAction())
                    return; // we could not process the event, it will be processed by our owner
                historyPush();
                generator_ = item->generator();
                ASSERT(generator_ != nullptr);
                delete menu_;
                menu_ = generator_();
                setItem(0, Direction::Up);
                // clear the button as we have processed the event already here
                btnClear(Btn::Up);
                btnClear(Btn::A);
            }
            if (btnDown(Btn::Down) || btnDown(Btn::B)) {
                if (previous_ != nullptr) {
                    btnClear(Btn::Down);
                    btnClear(Btn::B);
                    historyPop();
                } else {
                    return; // we could not process the event
                }
            }
            Carousel::processEvents();
        }

        typename Menu<PAYLOAD>::HistoryItem const * history() const {
            return previous_;
        }

        typename Menu<PAYLOAD>::HistoryItem * detachHistory() {
            // save current state
            historyPush();
            // and return the detached history
            auto result = previous_;
            previous_ = nullptr;
            return result;
        }

        void attachHistory(typename Menu<PAYLOAD>::HistoryItem * history) {
            ASSERT(previous_ == nullptr); // we do not expect reloading from history when there is history already present
            if (history == nullptr) {
                // no history, start with clean state
                // TODO really? 

            } else {
                previous_ = history;
                historyPop(Direction::Up);
            }
        }

    protected:

        /** Pushes current menu context to the history stack. 
         
            This is always simple operation where we just store the state.
         */
        void historyPush() {
            previous_ = new typename Menu<PAYLOAD>::HistoryItem{currentIndex(), generator_, previous_};
        }

        /** Leaves current submenu. 
        
         */
        void historyPop(Direction transition = Direction::Down) {
            ASSERT(previous_ != nullptr);
            auto h = previous_;
            previous_ = previous_->previous;
            generator_ = h->generator;
            // replace the menu
            delete menu_;
            menu_ = generator_();
            setItem(h->index, transition);
            // delete the history item;
            delete h;
        }
        
        void doSetItem(uint32_t index, Direction direction = Direction::None) override {
            ASSERT(menu_ != nullptr);
            if (menu_->size() == 0) {
                showEmpty(Direction::Up);
            } else {
                ASSERT(index < menu_->size());
                typename Menu<PAYLOAD>::MenuItem const & item = (*menu_)[index];
                set(item.text, item.icon, direction);
            }
        }

    private: 

        Menu<PAYLOAD> * menu_= nullptr;
        typename Menu<PAYLOAD>::MenuGenerator generator_ = nullptr;
        typename Menu<PAYLOAD>::HistoryItem * previous_ = nullptr;
    }; 

} // namespace rckid::ui