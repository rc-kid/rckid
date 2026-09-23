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
            Coord iconHeight = bmp.height();
            Coord textWidth = aText_->textWidth();
            // determine the final positions
            Coord iconLeft = (width() - (iconWidth + textWidth + ICON_SEPARATOR_WIDTH)) / 2;
            Coord textLeft = iconLeft + iconWidth + ICON_SEPARATOR_WIDTH;
            // and place & adjust the the widgets
            with(aImg_) 
                << SetBitmap(std::move(bmp))
                << SetRect(Rect::XYWH(iconLeft, (height() - iconHeight) / 2, iconWidth, iconHeight))
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
                        << MoveHorizontally(aImg_, aImg_->x() - width(), aImg_->x())->setDurationMs(animationSpeed())
                        << MoveHorizontally(aText_, aText_->x() + width(), aText_->x())->setDurationMs(animationSpeed())
                        << MoveVertically(bImg_, bImg_->y(), bImg_->y() + height())->setDurationMs(animationSpeed())
                        << MoveVertically(bText_, bText_->y(), bText_->y() + height())->setDurationMs(animationSpeed());
                    break;
                case Direction::Down:
                    // new comes from bottom, old goes to the sides
                    animate()
                        << MoveVertically(aImg_, aImg_->y() + height(), aImg_->y())->setDurationMs(animationSpeed())
                        << MoveVertically(aText_, aText_->y() + height(), aText_->y())->setDurationMs(animationSpeed())
                        << MoveHorizontally(bImg_, bImg_->x(), bImg_->x() - width())->setDurationMs(animationSpeed())
                        << MoveHorizontally(bText_, bText_->x(), bText_->x() + width())->setDurationMs(animationSpeed());
                    break;
                case Direction::Left:
                    // new comes from the left, old goes to the right
                    animate()
                        << MoveHorizontally(aImg_, aImg_->x() - width() * 2, aImg_->x())->setDurationMs(animationSpeed())
                        << MoveHorizontally(aText_, aText_->x() - width(), aText_->x())->setDurationMs(animationSpeed())
                        << MoveHorizontally(bImg_, bImg_->x(), bImg_->x() + width())->setDurationMs(animationSpeed())
                        << MoveHorizontally(bText_, bText_->x(), bText_->x() + width() * 2)->setDurationMs(animationSpeed());
                    break;
                case Direction::Right:
                    // new comes from the right, old goes to the left
                    animate()
                        << MoveHorizontally(aImg_, aImg_->x() + width(), aImg_->x())->setDurationMs(animationSpeed())
                        << MoveHorizontally(aText_, aText_->x() + width() * 2, aText_->x())->setDurationMs(animationSpeed())
                        << MoveHorizontally(bImg_, bImg_->x(), bImg_->x() - width() * 2)->setDurationMs(animationSpeed())
                        << MoveHorizontally(bText_, bText_->x(), bText_->x() - width())->setDurationMs(animationSpeed());
                    break;
                default:
                    UNREACHABLE;
            }
            RootWidget::backgroundEffect(dir, animationSpeed());
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

    }; // rckid::ui::Carousel


    class CarouselMenu : public Carousel {
    public:

        ui::Menu::Context & menu() { return menu_; }

        std::function<void()> onMenuChange;

        std::function<void(ui::MenuItem const &)> onItemSelected = [] (ui::MenuItem const & item) {
            if (item.isAction())
                item.action()();
        };

        void processEvents() {
            if (btnPressed(Btn::Left))
                moveLeft();
            if (btnPressed(Btn::Right))
                moveRight();  
            if (btnPressed(Btn::Up) || btnPressed(Btn::A))
                moveUp();
            if (btnPressed(Btn::Down) || btnPressed(Btn::B))
                moveDown();
        }

        void moveLeft() {
            if (menu_.empty() || menu_.menu()->empty())
                return;
            if (!idle())
                cancelAnimations();
            menu_.setIndex((menu_.index() + menu_.menu()->size() - 1) % menu_.menu()->size());
            ui::MenuItem const * mi = menu_.currentItem();
            setItem(mi, Direction::Left);     
        }

        void moveRight() {
            if (menu_.empty() || menu_.menu()->empty())
                return;
            if (!idle())
                cancelAnimations();
            menu_.setIndex((menu_.index() + 1) % menu_.menu()->size());
            ui::MenuItem const * mi = menu_.currentItem();
            setItem(mi, Direction::Right);            
        }

        void moveUp() {
            if (menu_.empty() || menu_.menu()->empty())
                return;
            if (!idle())
                cancelAnimations();
            ui::MenuItem const * mi = menu_.currentItem();
            if (mi->isAction()) {
                if (onItemSelected)
                    onItemSelected(*mi);
                // refresh & redecorate the current item
                ui::MenuItem const * item = menu_.currentItem();
                setItem(item);
            } else {
                enterMenu(mi->generator());
                if (onMenuChange)
                    onMenuChange();
            }
        }

        void moveDown(bool animate = true) {
            if (menu_.parent() != nullptr) {
                if (!idle())
                    cancelAnimations();
                menu_.pop();
                menu_.refresh();
                if (onMenuChange)
                    onMenuChange();
                ui::MenuItem const * mi = menu_.currentItem();
                if (animate)
                    setItem(mi, Direction::Down);
                else
                    setItem(mi);
            }
        }

        void enterMenu(ui::MenuItem::GeneratorEvent generator) {
            if (!idle())
                cancelAnimations();
            menu_.openNew(std::move(generator));
            menu_.populate();
            if (menu_.menu()->empty()) {
                setEmpty(Direction::Up);
            } else {
                ui::MenuItem const * mi = menu_.currentItem();
                setItem(mi, Direction::Up);
            }
        }

        /** Refreshes the curret menu.
         
            Repopulates the current menu context and resets & redecorates the current item. This is useful when we expect menu to change due to some action (e.g. entering or leaving a mode, etc.)
         */
        void refresh() {
            menu_.refresh();
            setItem(menu_.currentItem());
        }


    protected:

        void setItem(ui::MenuItem const * item, Direction dir) {
            set(item->text, item->icon, dir);
            if (item->decorator() != nullptr)
                item->decorator()(*item, currentImage(), currentLabel());            
        }

        /** Sets the item immediately without any animations.
         */
        void setItem(ui::MenuItem const * item) {
            set(item->text, item->icon);
            if (item->decorator() != nullptr)
                item->decorator()(*item, currentImage(), currentLabel());            
        }

        ui::Menu::Context menu_;

    }; // CarouselMenu

} // namespace rckid::ui