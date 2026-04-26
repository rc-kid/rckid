#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/menu.h>
#include <rckid/ui/scroll_view.h>
#include <rckid/ui/image.h>
#include <rckid/ui/label.h>

#include <assets/OpenDyslexic24.h>

namespace rckid {


    /** Popup Menu  
     */
    class PopupMenu : public ui::App<ui::MenuItem*> {
    public:

        String name() const override { return "PopupMenu"; }

        PopupMenu(ui::Menu * menu):
            menu_{menu} 
        {
            using namespace ui;
            Coord width = 0;
            bool images = false;
            Font f{assets::OpenDyslexic24};
            for (auto & i : *menu) {
                images = images | (! i.icon.empty());
                width = std::max(width, f->textWidth(i.text));
            }
            width += images ? 40 : 10;
            if (width < 100)
                width = 100;
            else if (width > display::WIDTH)
                width = display::WIDTH;
            rows_ = static_cast<Coord>(menu->size());
            Coord height = std::min(MAX_ROWS, rows_) * 24;
            root_.setRect(Rect::XYWH(0, display::HEIGHT - height, width, height));

            if (images)
                icons_ = unique_ptr<ui::Image*>(new ui::Image * [rows_]);
            labels_ = unique_ptr<ui::Label*>(new ui::Label * [rows_]);

            view_ = addChild(new ScrollView{})
                << SetRect(Rect::WH(width, height));

            sel_ = view_->addChild(new Panel{}) 
                << SetBg(Style::defaultStyle().accentFg())
                << SetRect(Rect::XYWH(2, 2, width - 4, 22));

            Style const & style = Style::defaultStyle();

            for (Coord i = 0; i < rows_; ++i) {
                MenuItem const & mi = menu->at(i);
                if (images) {
                    icons_.get()[i] = view_->addChild(new Image{})
                        << SetRect(Rect::XYWH(5, i * 24, 24, 24))
                        << SetBitmap(mi.icon);
                }
                labels_.get()[i] = view_->addChild(new Label{})
                    << SetRect(Rect::XYWH(images ? 30 : 5, i * 24, width - (images ? 35 : 5), 24))
                    << SetFont(f)
                    << SetText(mi.text)
                    << SetColorGradient(style.defaultFg(), style.accentBg());
            }

            root_.useBackgroundImage(false);
                
        }

    protected:

        void loop() override {
            ui::App<ui::MenuItem*>::loop();
            if (btnPressed(Btn::Up))
                moveUp();
            if (btnPressed(Btn::Down))
                moveDown();
            if (btnPressed(Btn::A))
                exit(& menu_->at(selRow_));
            if (btnPressed(Btn::B))
                exit();
        }

    private:

        static constexpr Coord MAX_ROWS = 7;

        void moveUp() {
            if (--selRow_ < 0) {
                selRow_ = menu_->size() - 1;
                rowOffset_ = std::max(0, selRow_ - MAX_ROWS + 1);
            } else if (selRow_ < rowOffset_) {
                --rowOffset_;
            }
            updatePosition();
        }

        void moveDown() {
            if (++selRow_ == static_cast<Coord>(menu_->size())) {
                selRow_ = 0;
                rowOffset_ = 0;
            } else if (selRow_ - rowOffset_ >= MAX_ROWS) {
                ++rowOffset_;
            }
            updatePosition();
        }

        void updatePosition() {
            animate()
                << ui::ScrollTo(view_, Point{0, rowOffset_ * 24})
                << ui::MoveTo(sel_, Point{2, 2 + selRow_ * 24});
        }

        Coord rows_;
        Coord rowOffset_ = 0;
        Coord selRow_ = 0;

        ui::ScrollView * view_;
        ui::Menu * menu_;
        unique_ptr<ui::Image *> icons_;
        unique_ptr<ui::Label *> labels_;
        ui::Panel * sel_;

    }; // rckid::PopupMenu


}