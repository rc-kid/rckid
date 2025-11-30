#pragma once

#include "../../ui/form.h"
#include "../../ui/tilemap.h"
#include "../../ui/geometry.h"
#include "../../utils/interpolation.h"
#include "../../utils/string.h"
#include "../../assets/tiles/System24.h"

namespace rckid {

    
    /** A simple dialog app that returns text written by the user via an on screen keyboard.
      
        Uses a simple tilemap and a sprite for the selection.  
     */
    class PinDialog : public ui::Form<uint16_t> {
    public:

        String name() const override { return "PinDialog"; }

        PinDialog(char const * placeholder = "Enter PIN"):
            ui::Form<uint16_t>{Rect::XYWH(0, 144, 320, 96), /* raw */ true},
            placeholder_{placeholder}
        {
            using namespace ui;
            g_.setBg(ColorRGB::White().withAlpha(32));
            icon_ = g_.addChild(new ui::Image{Rect::WH(110, 96), Icon{assets::icons_64::lock}});
            tileMap_ = g_.addChild(new ui::Tilemap<Tile<12,24,Color16>>{15, 4, assets::System24, palette_});
            selRect_ = g_.addChild(new ui::Rectangle{Rect::WH(24, 24)});
            tileMap_->setPos(110, 0);
            selRect_->setPos(118, 48);
            drawKeyboard();
            tileMap_->text(0, 0) << placeholder_;
        }

    protected:

        void drawKeyboard() {
            tileMap_->text(0, 2) << " 1 2 3 4 5 * < ";
            tileMap_->text(0, 3) << " 6 7 8 9 0 # x ";
        }

        void wrapSelectPosition() {
            if (select_.x > 6)
                select_.x = 0;
            if (select_.x < 0)
                select_.x = 6;
            if (select_.y < 0)
                select_.y = 1;
            if (select_.y > 1)
                select_.y = 0;
            last_ = selRect_->pos();
            a_.start();
        }

        void keyPress() {
            if (select_.y == 0) {
                switch (select_.x) {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                        insert(select_.x + 1);
                        break;
                    case 5: // star
                        insert(10);
                        break;
                    case 6: // backspace
                        if (length_ > 0) {
                            tileMap_->text(length_--, 0) << ' ';
                            pin_ >>= 4;
                            pin_ |= 0xf000;
                            if (length_ == 0) {
                                tileMap_->text(0, 0) << "               ";
                                tileMap_->text(0, 0) << placeholder_;
                            }
                        }
                        break;
                    default:
                        UNREACHABLE;
                }
            } else {
                switch (select_.x) {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                        insert(select_.x + 5);
                        break;
                    case 4:
                        insert(0);
                        break;
                    case 5: // hash
                        insert(11);
                        break;
                    case 6: // enter
                        exit(pin_);
                        break;
                    default:
                        UNREACHABLE;
                }
            }
        }

        void insert(uint8_t what) {
            if (length_ == 0)
                tileMap_->text(0, 0) << "               ";
            tileMap_->text(++length_, 0) << '-';
            pin_ <<= 4;
            pin_ |= what & 0x0f;
            if (length_ == 4)
                exit(pin_);
        }

        void update() {
            // check any keyboard actions
            ui::Form<uint16_t>::update();

            a_.update();
            int x = select_.x * 24 + 118;
            int y = select_.y * 24 + 48;
            if (a_.running()) {
                x = interpolation::cosine(a_, last_.x, x).round();
                y = interpolation::cosine(a_, last_.y, y).round();
            }
            selRect_->setPos(x, y);

            if (! a_.running()) {
                if (btnDown(Btn::Right)) {
                    ++select_.x;
                    wrapSelectPosition();
                }
                if (btnDown(Btn::Left)) {
                    --select_.x;
                    wrapSelectPosition();
                }
                if (btnDown(Btn::Down)) {
                    --select_.y;
                    wrapSelectPosition();
                }
                if (btnDown(Btn::Up)) {
                    ++select_.y;
                    wrapSelectPosition();
                }
                if (btnPressed(Btn::A))
                    keyPress();
            }   
        }
        
    private:

        Timer a_{250};

        uint32_t length_ = 0;
        String placeholder_;

        ui::Image * icon_;
        ui::Tilemap<Tile<12,24,Color16>> * tileMap_; 
        ui::Rectangle * selRect_;

        Point select_{0, 0}; 
        Point last_;

        uint16_t pin_ = 0xffff;

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
    }; // rckid::TextDialog

} // namespace rckid