#pragma once

#include "rckid/rckid.h"
#include "rckid/app.h"
#include "rckid/graphics/animation.h"
#include "rckid/ui/ui.h"


namespace rckid {

    /** Text input field with on-screen keyboard. 
     
        Uses the UITileEngine with ROM tiles for minimal footprint. All that is required in RAM is the tilemap and palette and so the entire widget fits in less than 1KB RAM.  
      */
    class TextInput : public GraphicsApp<UITileEngine<>> {
    public:
        enum class KeyboardType {
            UpperCase, 
            LowerCase,
            FirstUpper,
            NumbersAndSymbols,
        }; // TextInput::KeyboardType

        TextInput():
            GraphicsApp{UITileEngine<>{26, 4, UITiles::Tileset, nullptr}}, 
            palette_{new ColorRGB[32] } {
            g_.setPalette(palette_);
            drawKeyboard(keyboardType_);
            // initialize the palette
            for (uint8_t i = 0; i < 16; ++i) {
                palette_[i] = ColorRGB{(i << 4) + i, (i << 4) + i, (i << 4) + i};
                palette_[i + 16] = ColorRGB{(i << 4) + i, 0, 0};
            }
            // TODO the sprites should live in ROM and therefore the initialization is not necessary
            UITileEngine<>::Sprite & sprite = g_.addSprite(24, 24);
            sprite.fill(0);
            sprite.setX(7);
            sprite.setY(48);
            for (int i = 0; i < 24; ++i) {
                sprite.setPixelAt(i, 0, 1);
                sprite.setPixelAt(i, 1, 1);
                sprite.setPixelAt(i, 22, 1);
                sprite.setPixelAt(i, 23, 1);
                sprite.setPixelAt(0, i, 1);
                sprite.setPixelAt(1, i, 1);
                sprite.setPixelAt(22, i, 1);
                sprite.setPixelAt(23, i, 1);
            }
            // and the cursor itself
            UITileEngine<>::Sprite & cursorSprite = g_.addSprite(2, 24);
            cursorSprite.fill(1);
            cursorSprite.setPos(12,0);
            //placeholder_ = "Enter name:";
        }
        
    protected:

        void update() override {
            App::update();
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

        void wrapSelectPosition() {
            if (select_.x > 24)
                select_.x = 2 - (select_.y % 2);
            if (select_.x < 1)
                select_.x = 24 - (select_.y % 2);
            if (select_.y < 0)
                select_.y = 2;
            if (select_.y > 2)
                select_.y = 0;
            auto & sprite = g_.getSprite(0);
            last_.x = sprite.x();
            last_.y = sprite.y();
            a_.start();
        }

        static constexpr Point KEY_UNKNOWN{2,0};
        static constexpr Point KEY_BACKSPACE{24, 0};
        static constexpr Point KEY_ENTER{23, 1};
        static constexpr Point KEY_SPACE{20, 2};
        static constexpr Point KEY_SHIFT{3, 1};
        static constexpr Point KEY_KEYBOARD_MODE{1,1};
        static constexpr Point KEY_LEFT{2,2};
        static constexpr Point KEY_RIGHT{4,2};

        void keyPress() {
            if (select_ == KEY_UNKNOWN) {

            } else if (select_ == KEY_BACKSPACE) {
                cursorLeft();
                text_.erase(cursor_, 1);
            } else if (select_ == KEY_ENTER) {
                // TODO return the text
                // exit(text_);
            } else if (select_ == KEY_SPACE) {
                insertChar(' ');
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
            } else if (select_ == KEY_KEYBOARD_MODE) {
                if (keyboardType_ == KeyboardType::NumbersAndSymbols) {
                    keyboardType_ = keyboardBackup_;
                } else {
                    keyboardBackup_ = keyboardType_;
                    keyboardType_ = KeyboardType::NumbersAndSymbols;
                }
            } else if (select_ == KEY_LEFT) {
                cursorLeft();
            } else if (select_ == KEY_RIGHT) {
                cursorRight();
            } else {
                insertChar(g_.at(select_.x, select_.y + 1).c);
            }
        }

        void insertChar(char c) {
            text_.insert(cursor_, 1, c);
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
        }

        void cursorRight() {
            if (cursor_ >= static_cast<int>(text_.size()))
                return;
            ++cursor_;
            if (cursor_ - left_ > 24)
                ++left_;
        }

        void draw() override {
            a_.update();
            auto & sprite = g_.getSprite(0);
            auto & cursorSprite = g_.getSprite(1);
            pOffset = (pOffset + 1) & 0x1f;
            if (pOffset >= 0x10) {
                sprite.setPaletteOffset(47 - pOffset);
                cursorSprite.setPaletteOffset(47 - pOffset);
            } else {
                sprite.setPaletteOffset(16 + pOffset);
                cursorSprite.setPaletteOffset(16 + pOffset);
            }
            int x = select_.x * 12 - 6;
            int y = select_.y * 24 + 24;
            if (a_.running()) {
                x = a_.interpolate(last_.x, x);
                y = a_.interpolate(last_.y, y);
            }
            sprite.setPos(x, y);
            cursorSprite.setX(12 + (cursor_ - left_) * 12);
            drawKeyboard(keyboardType_);
            drawText();
        }

        void drawKeyboard(KeyboardType type) {
            switch (type) {
                case KeyboardType::UpperCase:
                case KeyboardType::FirstUpper:
                    g_.text(0, 1) << "  @ Q W E R T Y U I O P * ";
                    g_.text(0, 2) << " @ ^ A S D F G H J K L <  ";
                    g_.text(0, 3) << "  < > Z X C V B N M _ . , ";
                    break;
                case KeyboardType::LowerCase:
                    g_.text(0, 1) << "  @ q w e r t y u i o p * ";
                    g_.text(0, 2) << " @ ^ a s d f g h j k l <  ";
                    g_.text(0, 3) << "  < > z x c v b n m _ . , ";
                    break;
                case KeyboardType::NumbersAndSymbols:
                    g_.text(0, 1) << "  @ 1 2 3 4 5 6 7 8 9 @ * ";
                    g_.text(0, 2) << " @ ^ ( ) [ ] ! ? @ # $ <  ";
                    g_.text(0, 3) << "  < > % ^ & * ' \" ~ _ . , ";
                    break;
            }
        }

        void drawText() {
            g_.text(0,0) << "<                        >";
            if (text_.empty()) {
                cursor_ = 0;
                left_ = 0;
                g_.text(1, 0) << placeholder_;
            } else {
                g_.text(1, 0) << (text_.c_str() + left_);
            }
        }

        KeyboardType keyboardType_ = KeyboardType::UpperCase;
        KeyboardType keyboardBackup_; 

        // palette
        ColorRGB * palette_; 
        // palette offset for the selection and cursor
        uint8_t pOffset = 0;

        std::string text_;
        std::string placeholder_;

        Animation a_{250};
        // Center of the tile in the selection frame
        Point select_{4,0};
        // old sprite position to determine the animation
        Point last_; 
        // cursor and the left offset of the displayed text
        int cursor_ = 0;
        int left_ = 0;


    }; // TextInput


} // namespace rckid