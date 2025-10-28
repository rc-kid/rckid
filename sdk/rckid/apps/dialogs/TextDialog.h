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
    class TextDialog : public ui::Form<String> {
    public:

        String name() const override { return "TextDialog"; }

        enum class KeyboardType {
            UpperCase, 
            LowerCase,
            FirstUpper,
            NumbersAndSymbols,
        }; // TextInput::KeyboardType

        TextDialog():
            ui::Form<String>{Rect::XYWH(0, 144, 320, 96), /* raw */ true} {
                using namespace ui;
                g_.setBg(ColorRGB::White().withAlpha(32));
                tileMap_ = g_.addChild(new ui::Tilemap<Tile<12, 24, Color16>>{26, 4, assets::System24, palette_});
                tileMap_->setPos(4, 0);
                drawKeyboard(KeyboardType::UpperCase);
                drawText();
                selRect_ = g_.addChild(new ui::Rectangle{Rect::WH(24, 24)});
                cursorLine_ = g_.addChild(new ui::VLine{Rect::WH(24, 24)});
                cursorLine_->setX(16 + (cursor_ - left_) * 12);
            }

    protected:

        static constexpr Point KEY_UNKNOWN{2,0};
        static constexpr Point KEY_BACKSPACE{24, 0};
        static constexpr Point KEY_ENTER{23, 1};
        static constexpr Point KEY_SPACE{20, 2};
        static constexpr Point KEY_SHIFT{3, 1};
        static constexpr Point KEY_KEYBOARD_MODE{1,1};
        static constexpr Point KEY_LEFT{2,2};
        static constexpr Point KEY_RIGHT{4,2};

        void update() override {
            // if back button is selected, do return nullopt
            if (btnPressed(Btn::B))
                exit();
            // deal with the animation timer
            a_.update();
            int x = select_.x * 12 - 2;
            int y = select_.y * 24 + 24;
            if (a_.running()) {
                x = interpolation::cosine(a_, last_.x, x).round();
                y = interpolation::cosine(a_, last_.y, y).round();
            }
            selRect_->setPos(x, y);

            // check any keyboard actions
            ui::Form<String>::update();
            if (! a_.running()) {
                if (btnDown(Btn::Right)) {
                    select_.x += 2;
                    wrapSelectPosition();
                }
                if (btnDown(Btn::Left)) {
                    select_.x -= 2;
                    wrapSelectPosition();
                }
                if (btnDown(Btn::Down)) {
                    if (++select_.y <= 2)
                        ++select_.x;
                    wrapSelectPosition();
                }
                if (btnDown(Btn::Up)) {
                    if (--select_.y >= 0)
                        --select_.x;
                    wrapSelectPosition();
                }
                if (btnPressed(Btn::A))
                    keyPress();
                if (btnPressed(Btn::Start))
                    insertChar(' ');    
            }   
        }

        void insertChar(char c) {
            text_.insert(cursor_, c);
            if (keyboardType_ == KeyboardType::FirstUpper)
                keyboardType_ = KeyboardType::LowerCase;
            cursorRight();
        }

        void cursorLeft() {
            if (cursor_ <= 0)
                return;
            --cursor_;
            if (cursor_ < left_)
                --left_;
            drawText();
            cursorLine_->setX(16 + (cursor_ - left_) * 12);
        }

        void cursorRight() {
            if (cursor_ > static_cast<int>(text_.size()))
                return;
            ++cursor_;
            if (cursor_ - left_ > 24)
                ++left_;
            drawText();
            cursorLine_->setX(16 + (cursor_ - left_) * 12);
        }

        void keyPress() {
            if (select_ == KEY_UNKNOWN) {
                insertChar(' ');
            } else if (select_ == KEY_BACKSPACE) {
                cursorLeft();
                text_.erase(cursor_, 1);
                drawText();
            } else if (select_ == KEY_ENTER) {
                // return the text
                exit();
            //} else if (select_ == KEY_SPACE) {
            //    insertChar(' ');
            } else if (select_ == KEY_SHIFT) {
                switch (keyboardType_) {
                    case KeyboardType::UpperCase:
                        keyboardType_ = KeyboardType::LowerCase;
                        break;
                    case KeyboardType::LowerCase:
                        keyboardType_ = KeyboardType::FirstUpper;
                        break;
                    case KeyboardType::FirstUpper:
                        keyboardType_ = KeyboardType::UpperCase;
                        break;
                    default:
                        // TODO maybe normal button?
                        break;
                }
                drawKeyboard(keyboardType_);
            } else if (select_ == KEY_KEYBOARD_MODE) {
                if (keyboardType_ == KeyboardType::NumbersAndSymbols) {
                    keyboardType_ = keyboardBackup_;
                } else {
                    keyboardBackup_ = keyboardType_;
                    keyboardType_ = KeyboardType::NumbersAndSymbols;
                }
                drawKeyboard(keyboardType_);
            } else if (select_ == KEY_LEFT) {
                cursorLeft();
            } else if (select_ == KEY_RIGHT) {
                cursorRight();
            } else {
                insertChar(tileMap_->at(select_.x, select_.y + 1).c);
            }
        }

        void drawKeyboard(KeyboardType type) {
            switch (type) {
                case KeyboardType::UpperCase:
                case KeyboardType::FirstUpper:
                    tileMap_->text(0, 1) << "  \x1d Q W E R T Y U I O P * ";
                    tileMap_->text(0, 2) << " \x1c \x1b A S D F G H J K L \x1a  ";
                    tileMap_->text(0, 3) << "  < > Z X C V B N M _ . , ";
                    break;
                case KeyboardType::LowerCase:
                    tileMap_->text(0, 1) << "  \x1d q w e r t y u i o p * ";
                    tileMap_->text(0, 2) << " \x1c \x1b a s d f g h j k l \x1a  ";
                    tileMap_->text(0, 3) << "  < > z x c v b n m _ . , ";
                    break;
                case KeyboardType::NumbersAndSymbols:
                    tileMap_->text(0, 1) << "  \x1d 1 2 3 4 5 6 7 8 9 @ * ";
                    tileMap_->text(0, 2) << " \x1c \x1b ( ) [ ] ! ? @ # $ \x1a  ";
                    tileMap_->text(0, 3) << "  < > % ^ & * ' \" ~ _ . , ";
                    break;
            }
        }

        void drawText() {
            tileMap_->text(0,0) << "<                        >";
            if (text_.empty()) {
                cursor_ = 0;
                left_ = 0;
                tileMap_->text(1, 0) << placeholder_;
            } else {
                tileMap_->text(1, 0) << (text_.c_str() + left_);
            }
        }

        void wrapSelectPosition() {
            if (select_.x > 24)
                select_.x = 2 - (select_.y % 2);
            if (select_.x < 1)
                select_.x = 24 - (select_.y % 2);
            if (select_.y < 0)
                select_.y = 2;
            if (select_.y > 2)
                select_.y = 0;
            last_ = selRect_->pos();
            a_.start();
        }
        
    private:

        Timer a_{250};

        String text_;
        String placeholder_;

        KeyboardType keyboardType_ = KeyboardType::UpperCase;
        KeyboardType keyboardBackup_; 

        // cursor and the left offset of the displayed text
        int cursor_ = 0;
        int left_ = 0;

        ui::Tilemap<Tile<12,24,Color16>> * tileMap_; 
        ui::Rectangle * selRect_;
        ui::VLine * cursorLine_;

        Point select_{4, 0}; 
        Point last_;

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