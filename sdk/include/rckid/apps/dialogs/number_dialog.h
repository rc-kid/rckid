#pragma once

#include <rckid/ui/app.h>

#include <rckid/ui/label.h>
#include <rckid/ui/focus_rect.h>

#include <assets/OpenDyslexic24.h>
#include <assets/Iosevka24.h>

#include <assets/icons_16.h>

namespace rckid {

    /** Similar to text dialog, but shows only numeric keyboard
     */
    class NumberDialog : public ui::App<String> {
    public:
        String name() const override { return "NumberDialog"; }

        NumberDialog(String number = ""):
            ui::App<String>(Rect::XYWH(0, 140, 320, 100))
        {
            using namespace ui; 

            text_ = addChild(new Label{}) 
                << SetRect(Rect::XYWH(24, 0, 320 - 48, 24))
                << SetFont(assets::Iosevka24)
                << SetText(number)
                << SetHAlign(HAlign::Left);
            posText_ = text_->text().size();

            backspace_ = addChild(new Image{})
                << SetBitmap(assets::icons_16::backspace)
                << SetRect(getKeyPosition(4,0));

            enter_ = addChild(new Image{})
                << SetBitmap(assets::icons_16::play_button)
                << SetRect(getKeyPosition(4, 1));
            left_ = addChild(new Image{})
                << SetBitmap(assets::icons_16::back_arrow)
                << SetRect(getKeyPosition(3, 2));
            right_ = addChild(new Image{})
                << SetBitmap(assets::icons_16::right_arrow)
                << SetRect(getKeyPosition(4, 2));

            for (uint32_t i = 0; i < 12; ++i) {
                    keys_[i] = addChild(new Label{})
                        << SetHAlign(HAlign::Center)
                        << SetFont(assets::OpenDyslexic24);
            }

            setKeyboardLabels(keys_, "1230", 0, 0);
            setKeyboardLabels(keys_ + 4, "456-", 0, 1);
            setKeyboardLabels(keys_ + 8, "789", 0, 2);

            focus_ = addChild(new FocusRect{})
                << SetAnimationSpeed(100)
                << SetPadding(0);
            focus_->showAround(keys_[0], /* animate */ false);
            pos_ = Point{1, 0};

            cursor_ = addChild(new FocusRect{})
                << SetPadding(0)
                << SetFg(Color::Green())
                << SetRect(Rect::XYWH(24,2, 1, 20));
            updateCursorPosition();

            root_.useBackgroundImage(false);

        }

    private:

        static constexpr Coord CHAR_WIDTH = 9;
        static constexpr Coord OFFSET_LEFT = 100;

        Rect getKeyPosition(Coord col, Coord row) {
            return Rect::XYWH(
                OFFSET_LEFT + col * 24, 
                24 + row * 24,
                24,
                24    
            );
        }

        void setKeyboardLabels(ui::Label ** start, char const * labels, Coord startCol, Coord startRow) {
            using namespace ui;
            while (*labels != 0) {
                with(*start)
                    << SetRect(getKeyPosition(startCol, startRow))
                    << SetText(STR(*labels));
                ++labels;
                ++start;
                ++startCol;
            }
        }

        void loop() override {
            ui::App<String>::loop();
            if (btnPressed(Btn::B))
                exit();
            if (btnPressed(Btn::Left)) {
                if (--pos_.x < 0)
                    pos_.x = 4;
                focus_->showAt(getKeyPosition(pos_.x, pos_.y));
            }
            if (btnPressed(Btn::Right)) {
                if (++pos_.x > 4)
                        pos_.x = 0;
                focus_->showAt(getKeyPosition(pos_.x, pos_.y));
            }
            if (btnPressed(Btn::Up)) {
                if (--pos_.y < 0)
                    pos_.y = 2;
                focus_->showAt(getKeyPosition(pos_.x, pos_.y));
            }
            if (btnPressed(Btn::Down)) {
                if (++pos_.y > 2)
                    pos_.y = 0;
                focus_->showAt(getKeyPosition(pos_.x, pos_.y));
            }
            if (btnPressed(Btn::A))
                enterKey();
            if (btnPressed(Btn::B))
                exit();
        }

        void insertChar(char x) {
            text_->setText(text_->text().insertAt(posText_, x));
            ++posText_;
            updateCursorPosition();
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

        void enterKey() {
            switch (pos_.y) {
                case 0 : // 1 2 3 0 <
                    switch (pos_.x) {
                        case 0: insertChar('1'); break;
                        case 1: insertChar('2'); break;
                        case 2: insertChar('3'); break;
                        case 3: insertChar('4'); break;
                        case 4: // backspace
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
                            UNREACHABLE;
                    }
                    break;
                case 1 : // 4 5 6 - Enter
                    switch (pos_.x) {
                        case 0: insertChar('4'); break;
                        case 1: insertChar('5'); break;
                        case 2: insertChar('6'); break;
                        case 3: // minus / plus
                            if (posText_ == 0 && (text_->text().empty() || text_->text()[0] != '-')) {
                                insertChar('-');
                            }
                            // TODO else bad rumble
                            break;
                        case 4: // enter
                            exit(String{text_->text()});
                            break;
                        default:
                            UNREACHABLE;
                    }
                    break;
                case 2 : // 7 8 9 < >
                    switch (pos_.x) {
                        case 0: insertChar('7'); break;
                        case 1: insertChar('8'); break;
                        case 2: insertChar('9'); break;
                        case 3: // left
                            if (posText_ > 0) {
                                --posText_;
                                updateCursorPosition();
                            }
                            break;
                        case 4: // right
                            if (posText_ < static_cast<Coord>(text_->text().size())) {
                                ++posText_;
                                updateCursorPosition();
                            }
                            break;
                        default:
                            UNREACHABLE;
                    }
                    break;
                default:
                    UNREACHABLE;
            }
        }

    private:

        ui::Label * text_;
        ui::Label * keys_[11];
        ui::Image * backspace_;
        ui::Image * enter_;
        ui::Image * left_;
        ui::Image * right_;

        ui::FocusRect * focus_;
        ui::FocusRect * cursor_;

        Point pos_{1, 0};
        Coord posText_ = 0;
        Coord textOffset_ = 0;


    }; // rckid::NumberDialog

} // namespace rckid