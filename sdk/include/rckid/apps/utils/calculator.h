#pragma once

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <rckid/ui/app.h>
#include <rckid/ui/focus_rect.h>
#include <rckid/ui/label.h>

#include <assets/Iosevka24.h>
#include <assets/OpenDyslexic24.h>
#include <assets/OpenDyslexic64.h>

namespace rckid {

    /** Very simple calculator. 
     */
    class Calculator : public ui::App<void> {
    public:
        String name() const override { return "Calculator"; }

        Calculator() {
            using namespace ui;

            memoryLabel_ = addChild(new ui::Label{})
                << SetFont(assets::Iosevka24)
                << SetHAlign(HAlign::Right)
                << SetRect(Rect::XYWH(16, 20, 288, 24));

            displayLabel_ = addChild(new ui::Label{})
                << SetFont(assets::OpenDyslexic64)
                << SetFg(Style::defaultStyle().accentFg())
                << SetHAlign(HAlign::Right)
                << SetRect(Rect::XYWH(16, 48, 288, 64));

            keys_[0][3] = addChild(new Image{})
                << SetBitmap(assets::icons_16::backspace)
                << SetRect(keyRect(3, 0));

            for (Coord row = 0; row < KEY_ROWS; ++row) {
                for (Coord col = 0; col < KEY_COLS; ++col) {
                    if (keys_[row][col] != nullptr)
                        continue;
                    keys_[row][col] = addChild(new ui::Label{})
                        << SetFont(assets::OpenDyslexic24)
                        << SetHAlign(HAlign::Center)
                        << SetRect(keyRect(col, row))
                        << SetText(KEY_LABELS[row][col]);
                }
            }

            focus_ = addChild(new ui::FocusRect{})
                << SetPadding(0);
            focus_->showAround(keys_[0][0], false);

            root_.useBackgroundImage(false);
            syncLabels();
        }

    protected:

        void onLoopStart() override {
            ui::App<void>::onLoopStart();
            root_.flyIn();
        }

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::B)) {
                exit();
                waitUntilIdle();
                root_.flyOut();
                waitUntilIdle();
                return;
            }
            if (btnPressed(Btn::Left))
                moveFocus(-1, 0);
            if (btnPressed(Btn::Right))
                moveFocus(1, 0);
            if (btnPressed(Btn::Up))
                moveFocus(0, -1);
            if (btnPressed(Btn::Down))
                moveFocus(0, 1);
            if (btnPressed(Btn::A))
                enterKey(KEY_LABELS[pos_.y][pos_.x]);
            if (btnPressed(Btn::Start))
                enterKey("=");
        }

    private:
        static constexpr Coord KEY_COLS = 4;
        static constexpr Coord KEY_ROWS = 5;
        static constexpr Coord KEY_LEFT = 48;
        static constexpr Coord KEY_TOP = 116;
        static constexpr Coord KEY_WIDTH = 54;
        static constexpr Coord KEY_HEIGHT = 24;
        static constexpr Coord KEY_STEP_X = 56;
        static constexpr Coord KEY_STEP_Y = 24;
        static constexpr uint32_t MAX_INPUT_SIZE = 12;

        inline static constexpr char const * KEY_LABELS[KEY_ROWS][KEY_COLS] = {
            {"MC", "MR", "M+", "C"},
            {"7",  "8",  "9",  "/"},
            {"4",  "5",  "6",  "*"},
            {"1",  "2",  "3",  "-"},
            {"0",  ".",  "=",  "+"},
        };

        static Rect keyRect(Coord col, Coord row) {
            return Rect::XYWH(
                KEY_LEFT + col * KEY_STEP_X,
                KEY_TOP + row * KEY_STEP_Y,
                KEY_WIDTH,
                KEY_HEIGHT
            );
        }

        static String formatNumber(double value) {
            if (!std::isfinite(value))
                return "Error";
            if (value == 0.0)
                value = 0.0;
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "%.10g", value);
            return String{buffer};
        }

        double currentValue() const {
            return std::strtod(input_.c_str(), nullptr);
        }

        void syncLabels() {
            memoryLabel_->setText(STR("MR: " << formatNumber(memory_)));
            displayLabel_->setText(input_);
        }

        void clearState() {
            input_ = "0";
            storedValue_ = 0;
            pendingOperator_ = 0;
            hasStoredValue_ = false;
            startNewInput_ = false;
            error_ = false;
            syncLabels();
        }

        void enterError() {
            input_ = "Error";
            storedValue_ = 0;  
            pendingOperator_ = 0;
            hasStoredValue_ = false;
            startNewInput_ = true;
            error_ = true;
            syncLabels();
        }

        void moveFocus(Coord dx, Coord dy) {
            pos_.x += dx;
            pos_.y += dy;
            if (pos_.x < 0)
                pos_.x = KEY_COLS - 1;
            else if (pos_.x >= KEY_COLS)
                pos_.x = 0;
            if (pos_.y < 0)
                pos_.y = KEY_ROWS - 1;
            else if (pos_.y >= KEY_ROWS)
                pos_.y = 0;
            focus_->showAround(keys_[pos_.y][pos_.x]);
        }

        void appendDigit(char digit) {
            if (error_)
                clearState();
            if (startNewInput_) {
                input_ = STR(digit);
                startNewInput_ = false;
            } else if (input_ == "0") {
                input_ = STR(digit);
            } else if (input_.size() < MAX_INPUT_SIZE) {
                input_ = STR(input_ << digit);
            }
            syncLabels();
        }

        void appendDecimalPoint() {
            if (error_)
                clearState();
            for (uint32_t i = 0; i < input_.size(); ++i)
                if (input_[i] == '.')
                    return;
            if (startNewInput_) {
                input_ = "0.";
                startNewInput_ = false;
            } else if (input_.size() < MAX_INPUT_SIZE) {
                input_ = STR(input_ << ".");
            }
            syncLabels();
        }

        bool applyPending(double rhs) {
            double result = storedValue_;
            switch (pendingOperator_) {
                case '+':
                    result += rhs;
                    break;
                case '-':
                    result -= rhs;
                    break;
                case '*':
                    result *= rhs;
                    break;
                case '/':
                    if (rhs == 0.0) {
                        enterError();
                        return false;
                    }
                    result /= rhs;
                    break;
                default:
                    return true;
            }
            if (!std::isfinite(result)) {
                enterError();
                return false;
            }
            storedValue_ = result;
            input_ = formatNumber(result);
            startNewInput_ = true;
            syncLabels();
            return true;
        }

        void setOperator(char op) {
            if (error_)
                return;
            if (pendingOperator_ != 0 && hasStoredValue_) {
                if (startNewInput_) {
                    pendingOperator_ = op;
                    return;
                }
                if (!applyPending(currentValue()))
                    return;
            } else {
                storedValue_ = currentValue();
                hasStoredValue_ = true;
            }
            pendingOperator_ = op;
            startNewInput_ = true;
        }

        void evaluate() {
            if (error_ || pendingOperator_ == 0 || !hasStoredValue_)
                return;
            if (!applyPending(currentValue()))
                return;
            pendingOperator_ = 0;
            hasStoredValue_ = false;
        }

        void recallMemory() {
            if (error_)
                error_ = false;
            input_ = formatNumber(memory_);
            startNewInput_ = true;
            syncLabels();
        }

        void addToMemory() {
            if (error_)
                return;
            memory_ += currentValue();
            syncLabels();
        }

        void enterKey(char const * key) {
            if (std::strcmp(key, "MC") == 0) {
                memory_ = 0;
                syncLabels();
            } else if (std::strcmp(key, "MR") == 0) {
                recallMemory();
            } else if (std::strcmp(key, "M+") == 0) {
                addToMemory();
            } else if (std::strcmp(key, "C") == 0) {
                clearState();
            } else if (std::strcmp(key, ".") == 0) {
                appendDecimalPoint();
            } else if (std::strcmp(key, "=") == 0) {
                evaluate();
            } else if (std::strcmp(key, "+") == 0 || std::strcmp(key, "-") == 0 || std::strcmp(key, "*") == 0 || std::strcmp(key, "/") == 0) {
                setOperator(key[0]);
            } else {
                appendDigit(key[0]);
            }
        }

        ui::Label * memoryLabel_;
        ui::Label * displayLabel_;
        ui::Widget * keys_[KEY_ROWS][KEY_COLS]{};
        ui::FocusRect * focus_;

        Point pos_{0, 0};
        String input_{"0"};
        double storedValue_ = 0;
        double memory_ = 0;
        char pendingOperator_ = 0;
        bool hasStoredValue_ = false;
        bool startNewInput_ = false;
        bool error_ = false;

    }; // rckid::Calculator

} // namespace rckid
