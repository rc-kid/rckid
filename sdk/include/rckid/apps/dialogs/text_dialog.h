#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/focus_rect.h>

#include <assets/OpenDyslexic24.h>
#include <assets/Iosevka24.h>

#include <assets/icons_24.h>

namespace rckid {

    class TextDialog : public ui::App<String> {
    public:
        String name() const override { return "TextDialog"; }

        TextDialog(String value = ""):
            ui::App<String>(Rect::XYWH(0, 140, 320, 100))
        {
            using namespace ui;

            text_ = addChild(new Label{}) 
                << SetRect(Rect::XYWH(24, 0, 320 - 48, 24))
                << SetFont(assets::Iosevka24)
                << SetText(value)
                << SetHAlign(HAlign::Left);
            posText_ = text_->text().size();

            for (uint32_t i = 0; i < 29; ++i) {
                    keys_[i] = addChild(new Label{})
                        << SetHAlign(HAlign::Center)
                        << SetFont(assets::OpenDyslexic24);
            }

            space_ = addChild(new Image{})
                << SetBitmap(assets::icons_24::money_bag)
                << SetRect(Rect::XYWH(24, 24, 24, 24));
            backspace_ = addChild(new Image{})
                << SetBitmap(assets::icons_24::money_bag)
                << SetRect(Rect::XYWH(288, 24, 24, 24));
            keyMode_ = addChild(new Image{})
                << SetBitmap(assets::icons_24::money_bag)
                << SetRect(Rect::XYWH(12, 48, 24, 24));
            caps_ = addChild(new Image{})
                << SetBitmap(assets::icons_24::money_bag)
                << SetRect(Rect::XYWH(36, 48, 24, 24));
            enter_ = addChild(new Image{})
                << SetBitmap(assets::icons_24::money_bag)
                << SetRect(Rect::XYWH(276, 48, 24, 24));
            left_ = addChild(new Image{})
                << SetBitmap(assets::icons_24::money_bag)
                << SetRect(Rect::XYWH(24, 72, 24, 24));
            right_ = addChild(new Image{})
                << SetBitmap(assets::icons_24::money_bag)
                << SetRect(Rect::XYWH(48, 72, 24, 24));

            setKeyPosition(keys_, Point{48, 28}, 10);
            setKeyPosition(keys_ + 10, Point{60, 52}, 9);
            setKeyPosition(keys_ + 19, Point{72, 76}, 10);
            showKeyboardMode();

            focus_ = addChild(new FocusRect{})
                << SetPadding(0)
                << SetFg(Color::Green());
            focus_->showAround(keys_[0], /* animate */ false);
            pos_ = Point{1, 0};

            cursor_ = addChild(new FocusRect{})
                << SetPadding(0)
                << SetFg(Color::Green())
                << SetRect(Rect::XYWH(24,2, 1, 20));
            updateCursorPosition();
        }

    protected:
        void loop() override {
            ui::App<String>::loop();
            if (btnPressed(Btn::B)) {
                exit();
                // wait for idle to make sure we are exiting from known state
                //waitUntilIdle();
                //root_.flyOut();
                //waitUntilIdle();
            }
            if (btnPressed(Btn::Left)) {
                if (--pos_.x == -1)
                    pos_.x = pos_.y == 2 ? 10 : 11;
                focus_->showAround(getFocusedKey());
            }
            if (btnPressed(Btn::Right)) {
                if (++pos_.x == 12)
                    pos_.x = 0;
                focus_->showAround(getFocusedKey());
            }
            if (btnPressed(Btn::Up)) {
                if (--pos_.y == -1)
                    pos_.y = 2;
                focus_->showAround(getFocusedKey());
            }
            if (btnPressed(Btn::Down)) {
                if (++pos_.y == 3)
                    pos_.y = 0;
                focus_->showAround(getFocusedKey());
            }
            if (btnPressed(Btn::A)) {
                switch (pos_.y) {
                    case 0:
                        switch (pos_.x) {
                            case 0: // space
                                insertChar(' ');
                                break;
                            case 11: // backspace
                                if (posText_ > 0)
                                    --posText_;
                                text_->setText(
                                    STR(
                                        text_->text().substr(0, posText_) <<
                                        text_->text().substr(posText_ + 1)
                                    )
                                );
                                updateCursorPosition();
                                break;
                            default:
                                insertChar(keys_[pos_.x - 1]->text()[0]);
                                break;
                        }
                        break;
                    case 1:
                        switch (pos_.x) {
                            case 0: // keyboard mode
                                if (keyboardMode_ == KeyboardMode::Chars) {
                                    keyboardMode_ = KeyboardMode::Numbers;
                                    // TODO keyboard icon
                                } else {
                                    keyboardMode_ = KeyboardMode::Chars;
                                    // TODO keyboard icon
                                }
                                showKeyboardMode();
                                break;
                            case 1: // caps
                                if (keyboardMode_ == KeyboardMode::Chars) {
                                    switch (shiftMode_) {
                                        case ShiftMode::Normal:
                                            shiftMode_ = ShiftMode::FirstOnly;
                                            // TODO shift icon
                                            break;
                                        case ShiftMode::FirstOnly:
                                            shiftMode_ = ShiftMode::Caps;
                                            // TODO shift icon
                                            break;
                                        case ShiftMode::Caps:
                                            shiftMode_ = ShiftMode::Normal;
                                            // TODO shift icon
                                            break;
                                        default:
                                            UNREACHABLE;
                                    }
                                    showKeyboardMode();
                                }
                                break;
                            case 11: // enter
                                exit(String{text_->text()});
                                break;
                            default:
                                insertChar(keys_[10 + pos_.x - 2]->text()[0]);
                                break;
                        }
                        break;
                    case 2:
                        switch (pos_.x) {
                            case 0: // left
                                if (posText_ > 0) {
                                    --posText_;
                                    updateCursorPosition();
                                }
                                break;
                            case 1: // right
                                if (posText_ < static_cast<Coord>(text_->text().size())) {
                                    ++posText_;
                                    updateCursorPosition();
                                }
                                break;
                            default:
                                insertChar(keys_[19 + pos_.x - 2]->text()[0]);
                                break;
                        }
                        break;
                    default:
                        UNREACHABLE;
                }
            }
        }


    private:

        static constexpr Coord CHAR_WIDTH = 9;

        enum class KeyboardMode {
            Chars,
            Numbers,
        };

        enum class ShiftMode {
            Normal,
            FirstOnly,
            Caps,
        };

        // space q w e r t y u i o p BKSP 
        // key caps A S D 
        void showKeyboardMode() {
            switch (keyboardMode_) {
                case KeyboardMode::Chars:
                    switch (shiftMode_) {
                        case ShiftMode::Normal:
                            setKeyboardLabels(keys_,      "qwertyuiop");
                            setKeyboardLabels(keys_ + 10, "asdfghjkl");
                            setKeyboardLabels(keys_ + 19, "zxcvbnm_,.");
                            break;
                        default:
                            setKeyboardLabels(keys_,      "QWERTYUIOP");
                            setKeyboardLabels(keys_ + 10, "ASDFGHJKL");
                            setKeyboardLabels(keys_ + 19, "ZXCVBNM_,.");
                            break;
                    }
                    break;
                case KeyboardMode::Numbers:
                    setKeyboardLabels(keys_,      "1234567890");
                    setKeyboardLabels(keys_ + 10, "()[]!?@#$");
                    setKeyboardLabels(keys_ + 19, "%^&*'\"~_.,");
                    break;
                default:
                    UNREACHABLE;
            }
        }

        void setKeyboardLabels(ui::Label ** start, char const * labels) {
            while (*labels != 0) {
                (*start)->setText(STR(*labels));
                ++labels;
                ++start;
            }
        }

        void setKeyPosition(ui::Label ** start, Point pos, uint32_t num) {
            while (num > 0) {
                (*start)->setRect(Rect::XYWH(pos, 24, 24));
                pos+= Point{24, 0};
                ++start;
                --num;
            }
        }

        void focusLeft() {
            if (--pos_.x == -1)
                pos_.x = pos_.y == 2 ? 10 : 11;
        }

        void focusRight() {
            if (++pos_.x == 12)
                pos_.x = 0;
        }

        ui::Widget * getFocusedKey() const {
            switch (pos_.y) {
                case 0:
                    switch (pos_.x) {
                        case 0:
                            return space_;
                        case 11:
                            return backspace_;
                        default:
                            return keys_[pos_.x - 1];
                    }
                case 1:
                    switch (pos_.x) {
                        case 0:
                            return keyMode_;
                        case 1:
                            return caps_;
                        case 11:
                            return enter_;
                        default:
                            return keys_[10 + pos_.x - 2];
                    }
                case 2:
                    switch (pos_.x) {
                        case 0:
                            return left_;
                        case 1:
                            return right_;
                        default:
                            return keys_[19 + pos_.x - 2];
                    }
                default:
                    UNREACHABLE;
            }
        }

        void insertChar(char x) {
            text_->setText(text_->text().insertAt(posText_, x));
            ++posText_;
            updateCursorPosition();
            if (shiftMode_ == ShiftMode::FirstOnly) {
                shiftMode_ = ShiftMode::Normal;
                showKeyboardMode();
            }
        }

        void updateCursorPosition() {
            using namespace ui;
            if (posText_ < textOffset_) {
                --textOffset_;
                text_->setTextOffset(Point{-textOffset_ * CHAR_WIDTH, 0});
            } else if (posText_ - textOffset_ >  (320 - 48) / CHAR_WIDTH) {
                ++textOffset_;
                text_->setTextOffset(Point{-textOffset_ * CHAR_WIDTH, 0});
            }
            with(cursor_)
                << SetPosition(24 + (posText_ - textOffset_) * CHAR_WIDTH, 2);
        }

        KeyboardMode keyboardMode_ = KeyboardMode::Chars;

        ShiftMode shiftMode_ = ShiftMode::FirstOnly;
        
        ui::Label * text_;
        ui::Label * keys_[29];
        ui::Image * space_;
        ui::Image * backspace_;
        ui::Image * keyMode_;
        ui::Image * caps_;
        ui::Image * enter_;
        ui::Image * left_;
        ui::Image * right_;

        ui::FocusRect * focus_;
        ui::FocusRect * cursor_;

        Point pos_;
        Coord posText_ = 0;
        Coord textOffset_ = 0;

    }; // rckid::TextDialog

} // namespace rckid