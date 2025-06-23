#pragma once

#include "../../ui/form.h"
#include "../../ui/tilemap.h"
#include "../../ui/geometry.h"
#include "../../ui/label.h"
#include "../../utils/string.h"
#include "../../assets/icons_64.h"
#include "../../assets/fonts/OpenDyslexic32.h"


namespace rckid {

    /** Simple information dialog that can only be dismissed with either A or B buttons. 
        
        The dialog consists of optional icon, title and text. 
     */
    class InfoDialog : public ui::App<void> {
    public:
        InfoDialog(Icon const & icon, String title, String text):
            ui::App<void>{Rect::XYWH(0, 144, 320, 96), /* raw */ true} {
            using namespace ui;
            icon_ = g_.addChild(new ui::Image{Rect::WH(96, 96), icon});
            icon_->setTransparentColor(ColorRGB::Black());
            title_ = g_.addChild(new ui::Label{ 96, 5, std::move(title)});
            title_->setFont(Font::fromROM<assets::OpenDyslexic32>());
            text_ = g_.addChild(new ui::Tilemap<Tile<8, 16, Color16>>{27, 3, assets::System16, palette_});
            text_->setPos(96, 48);
            text_->textMultiline(0, 0) << text;
        }

        static void error(String title, String text) {
            NewArenaGuard g;
            ArenaAllocationGuard gg;
            InfoDialog * d  = new InfoDialog{Icon{assets::icons_64::poo}, std::move(title), std::move(text)};
            d->g_.setBg(ColorRGB::Red().withAlpha(32));
            d->run();
            delete d;
        }

        static void info(String title, String text) {
            NewArenaGuard g;
            ArenaAllocationGuard gg;
            InfoDialog * d  = new InfoDialog{Icon{assets::icons_64::book}, std::move(title), std::move(text)};
            d->run();
            delete d;
        }

        /** Info dialog is the only one that is *not* budgeted (we use it to inform user that we are out of budget.
         */
        bool isBudgeted() const override { return false; }

    protected:
        void update() override {
            ui::App<void>::update();
            // when back or down is pressed, return from the player mode
            if (btnPressed(Btn::B) || btnPressed(Btn::A))
                exit();
        }

    private:

      ui::Image * icon_;
      ui::Label * title_;
      ui::Tilemap<Tile<8, 16, Color16>> * text_;


        static constexpr uint16_t palette_[] = {
            // gray
            ColorRGB{0x00, 0x00, 0x00}.toRaw(), 
            ColorRGB{0x11, 0x11, 0x11}.toRaw(), 
            ColorRGB{0x22, 0x22, 0x22}.toRaw(), 
            ColorRGB{0x33, 0x33, 0x33}.toRaw(), 
            ColorRGB{0x44, 0x44, 0x44}.toRaw(), 
            ColorRGB{0x55, 0x55, 0x55}.toRaw(), 
            ColorRGB{0x66, 0x66, 0x66}.toRaw(), 
            ColorRGB{0x77, 0x77, 0x77}.toRaw(), 
            ColorRGB{0x88, 0x88, 0x88}.toRaw(), 
            ColorRGB{0x99, 0x99, 0x99}.toRaw(), 
            ColorRGB{0xaa, 0xaa, 0xaa}.toRaw(), 
            ColorRGB{0xbb, 0xbb, 0xbb}.toRaw(), 
            ColorRGB{0xcc, 0xcc, 0xcc}.toRaw(), 
            ColorRGB{0xdd, 0xdd, 0xdd}.toRaw(), 
            ColorRGB{0xee, 0xee, 0xee}.toRaw(), 
            ColorRGB{0xff, 0xff, 0xff}.toRaw(), 
            0, 
            ColorRGB{0xff, 0xff, 0xff}.toRaw(), 
            ColorRGB{0x00, 0xff, 0x00}.toRaw(),
        };


    }; // class rckid::InfoDialog


} // namespace rckid