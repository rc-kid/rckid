#pragma once

#include <rckid/ui/widget.h>
#include <rckid/ui/image.h>
#include <rckid/ui/label.h>
#include <rckid/ui/animation.h>
#include <rckid/ui/menu.h>

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
            if (!idle())
                cancelAnimations();
            setCurrent(std::move(text), std::move(icon));
            with(bImg_)
                << SetVisibility(false);
            with(bText_)
                << SetVisibility(false);
        }

        void set(String text, ImageSource icon, Direction dir) {
            if (!idle())
                cancelAnimations();
            setCurrent(std::move(text), std::move(icon));
            // start the appropriate animation
            switch (dir) {
                case Direction::Up:
                    // new comes from the sides, old goes down
                    animate()
                        << MoveHorizontally(aImg_, aImg_->x() - width(), aImg_->x(), animationSpeed_)
                        << MoveHorizontally(aText_, aText_->x() + width(), aText_->x(), animationSpeed_)
                        << MoveVertically(bImg_, bImg_->y(), bImg_->y() + height(), animationSpeed_)
                        << MoveVertically(bText_, bText_->y(), bText_->y() + height(), animationSpeed_);
                    break;
                case Direction::Down:
                    // new comes from bottom, old goes to the sides
                    animate()
                        << MoveVertically(aImg_, aImg_->y() + height(), aImg_->y(), animationSpeed_)
                        << MoveVertically(aText_, aText_->y() + height(), aText_->y(), animationSpeed_)
                        << MoveHorizontally(bImg_, bImg_->x(), bImg_->x() - width(), animationSpeed_)
                        << MoveHorizontally(bText_, bText_->x(), bText_->x() + width(), animationSpeed_);
                    break;
                case Direction::Left:
                    // new comes from the left, old goes to the right
                    animate()
                        << MoveHorizontally(aImg_, aImg_->x() - width() * 2, aImg_->x(), animationSpeed_)
                        << MoveHorizontally(aText_, aText_->x() - width(), aText_->x(), animationSpeed_)
                        << MoveHorizontally(bImg_, bImg_->x(), bImg_->x() + width(), animationSpeed_)
                        << MoveHorizontally(bText_, bText_->x(), bText_->x() + width() * 2, animationSpeed_);
                    break;
                case Direction::Right:
                    // new comes from the right, old goes to the left
                    animate()
                        << MoveHorizontally(aImg_, aImg_->x() + width(), aImg_->x(), animationSpeed_)
                        << MoveHorizontally(aText_, aText_->x() + width() * 2, aText_->x(), animationSpeed_)
                        << MoveHorizontally(bImg_, bImg_->x(), bImg_->x() - width() * 2, animationSpeed_)
                        << MoveHorizontally(bText_, bText_->x(), bText_->x() - width(), animationSpeed_);
                    break;
                default:
                    UNREACHABLE;
            }
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
            std::swap(aImg_, bImg_);
            std::swap(aText_, bText_);
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

    class CarouselMenu : public Carousel {
    public:

        Menu * menu() const { return menu_.get(); }

        uint32_t index() const { return index_; }

        bool empty() const { return menu_ == nullptr || menu_->size() == 0; }

        MenuItem * currentItem() const { return empty() ? nullptr : & menu_->at(index_); }

        void setMenu(unique_ptr<Menu> menu, uint32_t index = 0) {
            if (!idle())
                cancelAnimations();
            menu_ = std::move(menu);
            index_ = index;
            if (menu_ == nullptr)
                setEmpty();
            else
                set(menu_->at(index_).text, menu_->at(index_).icon);
        }

        void setMenu(unique_ptr<Menu> menu, Direction dir, uint32_t index = 0) {
            if (!idle())
                cancelAnimations();
            menu_ = std::move(menu);
            index_ = index;
            if (menu_ == nullptr)
                setEmpty(dir);
            else
                set(menu_->at(index_).text, menu_->at(index_).icon, dir);
        }

        void setItem(uint32_t index) {
            if (!idle())
                cancelAnimations();
            if (menu_ == nullptr || index >= menu_->size())
                return;
            index_ = index;
            set(menu_->at(index_).text, menu_->at(index_).icon);
        }

        void setItem(uint32_t index, Direction dir) {
            if (!idle())
                cancelAnimations();
            if (menu_ == nullptr || index >= menu_->size())
                return;
            index_ = index;
            set(menu_->at(index_).text, menu_->at(index_).icon, dir);
        }

        void moveLeft() {
            if (menu_ == nullptr || menu_->size() == 0)
                return;
            if (!idle())
                cancelAnimations();
            index_ = (index_ + menu_->size() - 1) % menu_->size();
            setItem(index_, Direction::Left);
        }

        void moveRight() {
            if (menu_ == nullptr || menu_->size() == 0)
                return;
            if (!idle())
                cancelAnimations();
            index_ = (index_ + 1) % menu_->size();
            setItem(index_, Direction::Right);
        }

    private:

        unique_ptr<Menu> menu_;
        uint32_t index_ = 0;

    }; // rckid::ui::CarouselMenu

    struct SetMenu {
        unique_ptr<Menu> menu;
        std::optional<Direction> direction;

        SetMenu(unique_ptr<Menu> menu, Direction direction):
            menu{std::move(menu)}, direction{direction} {
        }

        SetMenu(unique_ptr<Menu> menu):
            menu{std::move(menu)} {
        }
    };

    template<typename T>
    inline with<T> operator << (with<T> w, SetMenu && menu) {
        if (menu.direction.has_value())
            w->setMenu(std::move(menu.menu), menu.direction.value());
        else
            w->setMenu(std::move(menu.menu));
        return w;
    }


} // namespace rckid::ui