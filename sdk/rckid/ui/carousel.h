#pragma once

#include "../graphics/bitmap.h"

#include "timer.h"
#include "image.h"
#include "label.h"

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
            a_.update();
            // TODO 
        }

    protected:
        void renderColumn(Coord column, Pixel * buffer, Coord starty, Coord numPixels) override {
            renderChild(& aImg_, column, buffer, starty, numPixels);
            renderChild(& aText_, column, buffer, starty, numPixels);
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
            aOffset_ = 0;
            updateEffect();
        }

        void updateEffect() {
            switch (dir_) {
                case Btn::Home:
                    break;
                case Btn::Left:
                case Btn::Right:
                case Btn::Up:
                case Btn::Down:
                    UNIMPLEMENTED;
                    break;
            }
        }
    

    private:
        Image aImg_;
        Label aText_;
        Image bImg_;
        Label bText_;

        Btn dir_ = Btn::Home;
        Coord aOffset_;
        Coord bOffset_;
        Timer a_{defaultTransitionTimeMs};


    }; // rckid::ui::Carousel

} // namespace rckid::ui