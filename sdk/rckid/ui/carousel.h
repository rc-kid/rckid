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

        void set(Image icon, std::string text) {
            set(std::move(icon), std::move(text), aImg_, aText_);
        }

        void moveLeft(Image && icon, std::string text) {
            set(std::move(icon), std::move(text), bImg_, bText_);
            setEffect(Btn::Left);
        }

        void moveRight(Image && icon, std::string text) {
            set(std::move(icon), std::move(text), bImg_, bText_);
            setEffect(Btn::Right);
        }

        void moveUp(Image && icon, std::string text) {
            set(std::move(icon), std::move(text), bImg_, bText_);
            setEffect(Btn::Up);
        }

        void moveDown(Image && icon, std::string text) {
            set(std::move(icon), std::move(text), bImg_, bText_);
            setEffect(Btn::Down);
        }

        void update() override {
            if (dir_ == Btn::Home)
                return;
            if (a_.update()) {
                dir_ = Btn::Home;
                aImgOffset_ = 0;
                aTextOffset_ = 0;
                std::swap(aImg_, bImg_);
                std::swap(aText_, bText_);
            } else {
                updateOffsets();
            }
        }

    protected:
        void renderColumn(Coord column, Pixel * buffer, Coord starty, Coord numPixels) override {
            renderChild(& aImg_, column, buffer, starty, numPixels, Point(aImgOffset_, 0));
            renderChild(& aText_, column, buffer, starty, numPixels, Point(aTextOffset_, 0));
            if (dir_ != Btn::Home) {
                renderChild(& bImg_, column, buffer, starty, numPixels, Point(bImgOffset_, 0));
                renderChild(& bText_, column, buffer, starty, numPixels, Point(bTextOffset_, 0));
            }
        }

        void set(Image && icon, std::string &&text, Image & imgInto, Label & labelInto) {
            imgInto = std::move(icon);
            labelInto.setText(text);
            repositionElements(imgInto, labelInto);
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

        void setEffect(Btn effect) {
            dir_ = effect;
            aImgOffset_ = 0;
            aTextOffset_ = 0;
            a_.startContinuous();
            updateOffsets();
        }

        void updateOffsets() {
            switch (dir_) {
                // new item is coming from left, old goes to right
                case Btn::Left:
                    aImgOffset_ = interpolation::linear(a_, 0, width()).round();
                    aTextOffset_ = aImgOffset_ * 2;
                    bTextOffset_ = interpolation::linear(a_, -width(), 0).round();
                    bImgOffset_ = bTextOffset_ * 2;
                    break;
                // new item is coming from right, old goes to left
                case Btn::Right:
                    aTextOffset_ = interpolation::linear(a_, 0, -width()).round();
                    aImgOffset_ = aTextOffset_ * 2;
                    bImgOffset_ = interpolation::linear(a_, width(), 0).round();
                    bTextOffset_ = bImgOffset_ * 2;
                    break;
                // new item is coming from sides, old goes down
                case Btn::Up:
                    aTextOffset_ = interpolation::linear(a_, 0, height()).round();
                    aImgOffset_ = aTextOffset_;
                    bTextOffset_ = interpolation::linear(a_, width(), 0).round();
                    bImgOffset_ = - bTextOffset_;
                    break;
                // new item is coming the bottom, old goes to the sides
                case Btn::Down:
                    aTextOffset_ = interpolation::linear(a_, 0, width()).round();
                    aImgOffset_ = - aTextOffset_;
                    bTextOffset_ = interpolation::linear(a_, height(), 0).round();
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

        Btn dir_ = Btn::Home;
        Coord aImgOffset_;
        Coord aTextOffset_;
        Coord bImgOffset_;
        Coord bTextOffset_;
        Timer a_{defaultTransitionTimeMs * 10};
    }; // rckid::ui::Carousel


    /** Menu holder that visualizes a carousel. 
     
     */
    class CarouselMenu : public Carousel {
    public:

        CarouselMenu(Menu * m):
            menu_{m} {
            if (m->size() > 0) {
                set(menu()[0].icon(), menu.item(0)->text());
            }
        }

        Menu const * menu() const { return menu_; }
        void setMenu(Menu * m) {
            // TODO
        }

        /*
        CarouselMenu(Menu * m):
            menu_{m} {
            if (m->size() > 0) {
                set(menu()[0].icon(), menu.item(0)->text());
            }
        }

        Menu const & menu() const { return * menu_; }

        Menu & menu() { return * menu_; }

        void moveLeft() {
            if (menu_->size() == 0)
                return;
            menu_->moveLeft();
            moveLeft(menu_->item(0)->icon(), menu_->item(0)->text());
        }

        void moveRight() {
            if (menu_->size() == 0)
                return;
            menu_->moveRight();
            moveRight(menu_->item(0)->icon(), menu_->item(0)->text());
        }

        void moveUp() {
            if (menu_->size() == 0)
                return;
            menu_->moveUp();
            moveUp(menu_->item(0)->icon(), menu_->item(0)->text());
        }

        void moveDown() {
            if (menu_->size() == 0)
                return;
            menu_->moveDown();
            moveDown(menu_->item(0)->icon(), menu_->item(0)->text());
        }
        */



    private:

        Menu * menu_;

    }; // rckid::ui::CarouselMenu

} // namespace rckid::ui