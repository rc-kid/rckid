#pragma once

#include <rckid/ui/widget.h>
#include <rckid/ui/image.h>
#include <rckid/ui/label.h>
#include <rckid/ui/animation.h>
#include <rckid/ui/menu.h>
#include <rckid/ui/progress_bar.h>

#include <assets/OpenDyslexic64.h>
#include <assets/icons_64.h>

namespace rckid::ui {

    /** Carousel menu
     
        This is the main menu widget used for the main & home menus as well as most other selections in apps. Each carousel item consists of an Image and Widget (usually a label) displayed to the right of the image. 
     */
    class Carousel : public Widget {
    public:

        static constexpr Coord ICON_SEPARATOR_WIDTH = 5;

        Carousel():
            aImg_{addChild(new Image())},
            aText_{addChild(new Label())},
            bImg_{addChild(new Image())},
            bText_{addChild(new Label())} 
        {
            with(aImg_)
                << SetVAlign(VAlign::Center)
                << SetHAlign(HAlign::Center);
            with(bImg_)
                << SetVAlign(VAlign::Center)
                << SetHAlign(HAlign::Center);
            with(aText_)
                << SetVAlign(VAlign::Center)
                << SetHAlign(HAlign::Left)
                << SetFont(assets::OpenDyslexic64);
            with(bText_)
                << SetVAlign(VAlign::Center)
                << SetHAlign(HAlign::Left)
                << SetFont(assets::OpenDyslexic64);
        }

        Font font() const { return aText_->font(); }

        void setFont(Font font) {
            aText_->setFont(font);
            bText_->setFont(font);
        }

        uint32_t animationSpeed() const { return animationSpeed_; }

        void setAnimationSpeed(uint32_t speed) { animationSpeed_ = speed; }

        void setEmpty() {
            set("Empty", assets::icons_64::empty_box);
        }

        void setEmpty(Direction dir) {
            set("Empty", assets::icons_64::empty_box, dir);
        }

        void set(String text, ImageSource icon) {
            setCurrent(std::move(text), std::move(icon));
            with(bImg_)
                << SetVisibility(false);
            with(bText_)
                << SetVisibility(false);
        }

        void set(String text, ImageSource icon, Direction dir) {
            setCurrent(std::move(text), std::move(icon));
            // start the appropriate animation
            startAnimation(dir);
        }

        /** Returns the image and label widgets for the currently selected element. 
         
            When idle, those are the visible elements. During animation those are the elements that are going away.
         */
        //@{
        Image * currentImage() const { return aImg_; }
        Label * currentLabel() const { return aText_; }
        //@}

    protected:

        void onIdle() override {
            // when animations are done, hide the previous elements
            with(bImg_)
                << SetVisibility(false);
            with(bText_)
                << SetVisibility(false);
        }

        void setCurrent(String text, ImageSource icon) {
            if (!idle())
                cancelAnimations();
            std::swap(aImg_, bImg_);
            std::swap(aText_, bText_);
            // clear any decorations (children) that may exist in the widgets
            aImg_->clearChildren();
            aText_->clearChildren();
            // first set up the widgets so that we can calculate their size
            Bitmap bmp{std::move(icon)};
            with(aText_)
                << SetText(std::move(text));
            Coord iconWidth = bmp.width();
            Coord textWidth = aText_->textWidth();
            // determine the final positions
            Coord iconLeft = (width() - (iconWidth + textWidth + ICON_SEPARATOR_WIDTH)) / 2;
            Coord textLeft = iconLeft + iconWidth + ICON_SEPARATOR_WIDTH;
            // and place & adjust the the widgets
            with(aImg_) 
                << SetBitmap(std::move(bmp))
                << SetRect(Rect::XYWH(iconLeft, 0, iconWidth, height()))
                << SetVisibility(true);
            with(aText_)
                << SetRect(Rect::XYWH(textLeft, 0, width() - textLeft, height())) 
                << SetVisibility(true);
        }

        void startAnimation(Direction dir) {
            switch (dir) {
                case Direction::Up:
                    // new comes from the sides, old goes down
                    animate()
                        << MoveHorizontally(aImg_, aImg_->x() - width(), aImg_->x())->setDurationMs(animationSpeed_)
                        << MoveHorizontally(aText_, aText_->x() + width(), aText_->x())->setDurationMs(animationSpeed_)
                        << MoveVertically(bImg_, bImg_->y(), bImg_->y() + height())->setDurationMs(animationSpeed_)
                        << MoveVertically(bText_, bText_->y(), bText_->y() + height())->setDurationMs(animationSpeed_);
                    break;
                case Direction::Down:
                    // new comes from bottom, old goes to the sides
                    animate()
                        << MoveVertically(aImg_, aImg_->y() + height(), aImg_->y())->setDurationMs(animationSpeed_)
                        << MoveVertically(aText_, aText_->y() + height(), aText_->y())->setDurationMs(animationSpeed_)
                        << MoveHorizontally(bImg_, bImg_->x(), bImg_->x() - width())->setDurationMs(animationSpeed_)
                        << MoveHorizontally(bText_, bText_->x(), bText_->x() + width())->setDurationMs(animationSpeed_);
                    break;
                case Direction::Left:
                    // new comes from the left, old goes to the right
                    animate()
                        << MoveHorizontally(aImg_, aImg_->x() - width() * 2, aImg_->x())->setDurationMs(animationSpeed_)
                        << MoveHorizontally(aText_, aText_->x() - width(), aText_->x())->setDurationMs(animationSpeed_)
                        << MoveHorizontally(bImg_, bImg_->x(), bImg_->x() + width())->setDurationMs(animationSpeed_)
                        << MoveHorizontally(bText_, bText_->x(), bText_->x() + width() * 2)->setDurationMs(animationSpeed_);
                    break;
                case Direction::Right:
                    // new comes from the right, old goes to the left
                    animate()
                        << MoveHorizontally(aImg_, aImg_->x() + width(), aImg_->x())->setDurationMs(animationSpeed_)
                        << MoveHorizontally(aText_, aText_->x() + width() * 2, aText_->x())->setDurationMs(animationSpeed_)
                        << MoveHorizontally(bImg_, bImg_->x(), bImg_->x() - width() * 2)->setDurationMs(animationSpeed_)
                        << MoveHorizontally(bText_, bText_->x(), bText_->x() - width())->setDurationMs(animationSpeed_);
                    break;
                default:
                    UNREACHABLE;
            }
            RootWidget::backgroundEffect(dir, animationSpeed_);
        }

        /** Returns the image and label widgets for the next element. 
         
            Those elements are not visible when idle. When animated, those elements are getting into the view. When the animation is finished, those widgets will replace the current mage and label. 
         */

         //@{
        Image * prevImage() const { return bImg_; }
        Label * prevLabel() const { return bText_; }
        //@}

    private:
        Image * aImg_ = nullptr;
        Label * aText_ = nullptr;
        Image * bImg_ = nullptr;
        Label * bText_ = nullptr;

        uint32_t animationSpeed_ = 500; 

    }; // rckid::ui::Carousel

    /** Carousel augmented specifically for menu hierarchies.
     
        Provides handling of left/right key press to cycle through the menu, up or A to select submenu and B or down to return from the submenu. All other presses (i.e. A or up on item and B on root) are not cleared so that they can be processed by the application. 

        The carousel also supports subwidgets. Subwidgets temporarily display themselves under the menu label and take the user input actions instead of the main carousel. This is very useful for small settings such as as sliders, etc.
     */
    class CarouselMenu : public Carousel {
    public:

        struct Context {
        public:
            uint32_t index = 0;
            MenuItem::GeneratorEvent generator;
            Context * previous;
        private:
            friend class CarouselMenu;
            Context(MenuItem::GeneratorEvent generator, Context * previous):
                generator{generator}, previous{previous} {}
        }; 

        ~CarouselMenu() override {
            clearContext();
        }

        Menu * menu() const { return menu_.get(); }

        uint32_t index() const { return index_; }

        /** Returns previous index (wrapped at carousel start). 
         */
        uint32_t prevIndex() const {
            if (menu_ == nullptr || menu_->size() == 0)
                return 0;
            return (index_ + menu_->size() - 1) % menu_->size();
        }

        /** Returns next index (wrapped at carousel end)
         */
        uint32_t nextIndex() const {
            if (menu_ == nullptr || menu_->size() == 0)
                return 0;
            return (index_ + 1) % menu_->size();
        }

        Context const * context() const { return context_; }

        bool empty() const { return menu_ == nullptr || menu_->size() == 0; }

        MenuItem * currentItem() const { return empty() ? nullptr : & menu_->at(index_); }

        /** Moves the carousel immediately to the given item.
         */
        void setItem(uint32_t index) {
            if (menu_ == nullptr || index >= menu_->size())
                return;
            index_ = index;
            MenuItem & m = menu_->at(index_);
            set(m.text, m.icon);
            if (m.decorator() != nullptr)
                m.decorator()(m, currentImage(), currentLabel());
        }

        void resetMenu(MenuItem::GeneratorEvent generator) {
            clearContext();
            moveUp(generator);
        }

        void moveLeft() {
            if (menu_ == nullptr || menu_->size() == 0)
                return;
            if (!idle())
                cancelAnimations();
            index_ = prevIndex();
            setItem(index_, Direction::Left);
        }

        void moveRight() {
            if (menu_ == nullptr || menu_->size() == 0)
                return;
            if (!idle())
                cancelAnimations();
            index_ = nextIndex();
            setItem(index_, Direction::Right);
        }

        void moveUp(MenuItem::GeneratorEvent generator) {
            if (!idle())
                cancelAnimations();
            if (context_ != nullptr)
                context_->index = index_;
            if (generator == nullptr)
                generator = emptyMenuGenerator;
            context_ = new Context{generator, context_};
            setMenu(context_->generator(), context_->index, Direction::Up);
        }

        void moveDown() {
            ASSERT(context_ != nullptr);
            // do not do anything if there is nowhere to go
            if (context_->previous == nullptr)
                return;
            if (!idle())
                cancelAnimations();
            Context * old = context_;
            context_ = context_->previous;
            delete old;
            setMenu(context_->generator(), context_->index, Direction::Down);
        }

        /** Shows the given subwidget as part of the menu. The subwidget will modally respond to the carousel's controls. 
         */
        void showSubwidget(unique_ptr<Widget> subwidget) {
            ASSERT(subWidget_ == nullptr);
            ui::Label * l = currentLabel();
            // set position for the widget so that it appears under the label
            subWidget_ = l->addChild(subwidget.release())
                << SetPosition(0, l->textOffset().y + l->font()->size - 6);
            // determine the horizontal shift
            Coord hShift = (subWidget_->width() - l->textWidth()) / 2;
            if (hShift < 0)
                hShift = 0;
            // start the animation
            subWidgetFlyOut_ = false;
            animate()
                << ui::FlyOut(currentImage(), Point{-hShift, 0})
                << ui::FlyIn(subWidget_, Point{-hShift, l->height()})
                << ui::FlyOut(l, Point{-hShift, -subWidget_->height() / 2 - 6});
        }

        void hideSubwidget() {
            if (subWidget_ == nullptr)
                return;
            ui::Label * l = currentLabel();
            // determine the horizontal shift
            Coord hShift = (subWidget_->width() - l->textWidth()) / 2;
            if (hShift < 0)
                hShift = 0;
            // start the animations
            subWidgetFlyOut_ = true;
            animate()
                << ui::FlyOut(currentImage(), Point{hShift, 0})
                << ui::FlyOut(subWidget_, Point{hShift, l->height()})
                << ui::FlyOut(l, Point{hShift, subWidget_->height() / 2 + 6});
            // the subwidget will be deleted when the animation is done
        }

        /** Returns true if the carousel is at its root level.
         */
        bool atRoot() const {
            return context_->previous == nullptr;
        }

        void clearContext(Context const * until = nullptr) {
            while (context_ != until && context_ != nullptr) {
                Context * c = context_;
                context_ = context_->previous;
                delete c;
            }
            ASSERT(context_ == until || context_ == nullptr);
        }

        void processEvents() override {
            Carousel::processEvents();
            // if we have subwidget registered, let it handle the events instead
            if (subWidget_ != nullptr) {
                subWidget_->processEvents();
                // clear the button events the same way carousel would (this is not menu select from app's perspective)
                btnClear(Btn::Up);
                btnClear(Btn::Down);
                btnClear(Btn::A);
                btnClear(Btn::B);
                // no more processing of inputs here
                return;
            }
            // no subwidget, process carousel navigation
            if (btnPressed(Btn::Left))
                moveLeft();
            if (btnPressed(Btn::Right))
                moveRight();
            if (btnPressed(Btn::Up) || btnPressed(Btn::A)) {
                if (processMoveUp()) {
                    btnClear(Btn::Up);
                    btnClear(Btn::A);
                }
            }
            if (btnPressed(Btn::Down) || btnPressed(Btn::B)) {
                if (processMoveDown()) {
                    btnClear(Btn::B);
                    btnClear(Btn::Down);
                }
            }
        }

    protected:

        bool processMoveUp() {
            auto item = currentItem();
            // if there is nothing to do there is nothing to do even for the main app (empty carousel)
            if (item == nullptr)
                return true;
            // if the item is action, let the app handle it
            if (item->isAction()) 
                return false;
            // otherwise its generator, we can handle it ourselves
            moveUp(item->generator());
            return true;
        }

        bool processMoveDown() {
            if (atRoot())
                return false;
            moveDown();
            return true;
        }

        void setMenu(unique_ptr<Menu> menu, uint32_t index, Direction dir) {
            menu_ = std::move(menu);
            index_ = index;
            if (menu_ == nullptr || menu_->empty()) {
                ASSERT(index_ == 0);
                index_ = 0;
                setEmpty(dir);
            } else {
                setItem(index_, dir);
            }
        }

        void setItem(uint32_t index, Direction dir) {
            if (menu_ == nullptr || menu_->empty())
                return;
            if (index >= menu_->size())
                index = menu_->size() - 1;
            index_ = index;
            MenuItem & m = menu_->at(index_);
            set(m.text, m.icon, dir);
            if (m.decorator() != nullptr)
                m.decorator()(m, currentImage(), currentLabel());
        }

        void onIdle() override {
            Carousel::onIdle();
            if ((subWidget_ != nullptr) && subWidgetFlyOut_) {
                currentLabel()->clearChildren();
                subWidget_ = nullptr; // deleted by clear chilren above
                subWidgetFlyOut_ = false;
            }
        }

    private:

        // empty menu generator (used when the carousel is borrowed by the next app to show empty menu w/o any visible items, but we need to provide an empty item so that the empty menu visualiation inside carousel will not trigger)
        static unique_ptr<ui::Menu> emptyMenuGenerator() {
            auto result = std::make_unique<ui::Menu>();
            (*result)
                << ui::MenuItem{"", []() {
                    UNREACHABLE;
                }};
            return result;
        }

        unique_ptr<Menu> menu_;
        uint32_t index_ = 0;
        Context * context_ = nullptr;

        Widget * subWidget_ = nullptr;
        bool subWidgetFlyOut_ = false;

    }; // rckid::ui::CarouselMenu


    /** Basic carousel subwidget template.
     
        Remembers the parent carousel and and triggers sub widget hide when buttons down, or B are pressed. For actual in menu functionality, use the subclasses with actual UI implementation, or define own.
     */
    class CarouselSubWidget : public Widget {
    public:

        CarouselSubWidget(CarouselMenu * menu):
            menu_{menu} {
            ASSERT(menu != nullptr);
        }

        void processEvents() override {
            Widget::processEvents();
            if (btnPressed(Btn::Down) || btnPressed(Btn::B))
                menu_->hideSubwidget();
        }

    private:
        CarouselMenu * menu_;

    }; // rckid::ui::CarouselSubwidget

    /** Carousel subwidget that contains a single progress bar.
     */
    class ProgressBarSubWidget : public CarouselSubWidget {
    public:

        ProgressBarSubWidget(CarouselMenu * menu):
            CarouselSubWidget{menu} {
            bar_ = addChild(new ProgressBar{});
            setRect(Rect::WH(200, 20));
        }

        ProgressBarSubWidget(CarouselMenu * menu, int32_t min, int32_t max, int32_t value, std::function<void(int32_t)> onValueChange):
            ProgressBarSubWidget(menu) {
            with(bar_)
                << SetRange(min, max)
                << SetValue(value);
            setOnValueChange(onValueChange);
        }

        int32_t min() const { return bar_->min(); }
        int32_t max() const { return bar_->max(); }
        int32_t value() const { return bar_->value(); };

        void setMin(int32_t min) { bar_->setMin(min); }
        void setMax(int32_t max) { bar_->setMax(max); }
        void setValue(int32_t value) { bar_->setValue(value); }

        void setOnValueChange(std::function<void(int32_t)> f) { onValueChange_ = f; }

        void processEvents() override {
            CarouselSubWidget::processEvents();
            if (btnPressed(Btn::Left)) 
                if (bar_->changeValueBy(-1) && onValueChange_)
                    onValueChange_(bar_->value());
            if (btnPressed(Btn::Right) && onValueChange_) 
                if (bar_->changeValueBy(1))
                    onValueChange_(bar_->value());
        }

    protected:
        void onChange() override {
            CarouselSubWidget::onChange();
            bar_->setRect(Rect::WH(width(), height()));
        }

    private:

        ProgressBar * bar_;

        std::function<void(int32_t)> onValueChange_;

    }; // rckid::ui::ProgressBarSubwidget

    /** Carousel menu reset - clears the context and sets the menu to the provided value.
     */
    struct ResetMenu {
        MenuItem::GeneratorEvent menu;

        ResetMenu(MenuItem::GeneratorEvent menu): menu{std::move(menu)} { }

    };

    template<typename T>
    inline with<T> operator << (with<T> w, ResetMenu && menu) {
        w->resetMenu(std::move(menu.menu));
        return w;
    }

    struct OnValueChange {
        std::function<void(int32_t)> f;
        OnValueChange(std::function<void(int32_t)> f): f{f} {}
    }; 

    template<typename T>
    inline with<T> operator << (with<T> w, OnValueChange && event) {
        w->setOnValueChange(std::move(event.f));
        return w;
    }


} // namespace rckid::ui