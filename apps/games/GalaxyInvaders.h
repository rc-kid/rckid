#pragma once

#include <algorithm>

#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/graphics/sprite.h>
#include <rckid/ui/header.h>
#include <rckid/ui/alert.h>
#include <rckid/audio/tone.h>
#include <rckid/audio/music.h>

namespace rckid {

    /** SpaceInvaders-like simple game. 

     */
    class GalaxyInvaders : public GraphicsApp<Canvas<ColorRGB>> {
    public:

        static void run() {
            GalaxyInvaders g;
            g.resetGame();
            g.loop();
        }

    protected:

        GalaxyInvaders(): GraphicsApp{Canvas<Color>{320, 240}} {
            music_.setSheet(melody_);
            audio_[3].setWaveform(Tone::Waveform::Sine);
            audio_[3].setEnvelope(20, 10, 80, 100);
            music_.attachTo(audio_[3]);
            audio_[1].setEnvelope(100, 50, 80, 250);
        }

        void onFocus() override {
            GraphicsApp::onFocus();
            audioOn();
            audio_.enable();
        }

        void onBlur() override {
            GraphicsApp::onBlur();
            audio_.disable();
            audioOff();
        }

        void update() override {
            // handle back button
            GraphicsApp::update();
            bool moving = false;
            if (btnDown(Btn::Left)) {
                if (shipSpeed_ >= 0)
                    shipSpeed_ = -1;
                else if (shipSpeed_ > -10)
                    shipSpeed_ = shipSpeed_ * FixedInt{1, 0x1};
                shipX_ = shipX_ + shipSpeed_;
                if (shipX_ < 0)
                    shipX_ = 0;
                moving = true;
            }
            if (btnDown(Btn::Right)) {
                if (shipSpeed_ <= 0)
                    shipSpeed_ = 1;
                else if (shipSpeed_ < 10)
                    shipSpeed_ = shipSpeed_ * FixedInt{1, 0x1};
                shipX_ = shipX_ + shipSpeed_;
                if (shipX_ > 296)
                    shipX_ = 296;
                moving = true;
            }
            // fire
            if (btnPressed(Btn::A)) {
                bullets_.push_back(Point{shipX_ + 12, 200});
                audio_[0].setFrequency(440, 100);
            }
            if (!moving)
                shipSpeed_ = 0;
            // update the position if aliens and calculate any bullets
            aliens_.advance();
            advanceBullets();
        }

        void draw() override {
            g_.fill();
            drawBullets(bullets_, color::White);
            drawBullets(alienBullets_, color::Red);
            drawSpaceship();
            aliens_.drawOn(g_);
        }

        void resetGame() {
            score_ = 0;
            shipX_ = 160;
            aliens_.reset();
        }

        void drawSpaceship() {
            g_.fill(color::Green, Rect::XYWH(shipX_, 200, 24, 24));
        }

        void drawBullets(std::vector<Point> const & bullets, Color color) {
            for (auto & pos : bullets)
                g_.fill(color, Rect::XYWH(pos.x - 2, pos.y - 5, 4, 10));
        }

        void advanceBullets() {
            // advance bullets and check those that hit aliens
            for (auto & b: bullets_) {
                b.y -= 2;
                // check if the bullet hits an alien
                if (aliens_.checkBullet(b)) {
                    b.y = -100;
                    audio_[1].setFrequency(50, 500);
                    rumbleNudge();
                }
            }
            // remove any off screen bullets
            bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(), [](Point const & p) {
                 return p.y < 0; 
            }), bullets_.end());
        }

        void advanceAlienBullets() {
            // advance bullets and check those that hit aliens
            for (auto & b: alienBullets_) {
                b.y += 2;
                // TODO check if the bullet hits player
            }
            // remove any off screen bullets
            alienBullets_.erase(std::remove_if(alienBullets_.begin(), alienBullets_.end(), [](Point const & p) {
                 return p.y > 220; 
            }), alienBullets_.end());
        }

        uint32_t score_;

        FixedInt shipX_;
        FixedInt shipSpeed_;
        int32_t aliensX_;
        int32_t aliensY_;

        static constexpr int ALIEN_COLS = 8;
        static constexpr int ALIEN_ROWS = 5;

        class Aliens {
        public:
            FixedInt x;
            FixedInt speed{0, 0x8};
            int y;
            bool valid[ALIEN_COLS * ALIEN_ROWS];
            int freeLeft = 0;
            int freeRight = 0;

            void reset() {
                for (uint32_t i = 0; i < ALIEN_COLS * ALIEN_ROWS; ++i)
                    valid[i] = true;
                x = 160 - ALIEN_COLS * 10;
                y = 100;
            }

            bool isValid(int col, int row) {
                return valid[row + col * ALIEN_ROWS];
            }

            void remove(int col, int row) {
                valid[row + col * ALIEN_ROWS] = false;
                // when removing an alien, we need to check if the freeLeft and freeRight change
                if (col == freeLeft) {
                    while (freeLeft < ALIEN_COLS && isColumnEmpty(freeLeft))
                    ++freeLeft;
                }
                if (col == ALIEN_COLS - freeRight - 1) {
                    while (freeRight < ALIEN_COLS && isColumnEmpty(ALIEN_COLS - freeRight - 1))
                        ++freeRight;
                }
            }

            bool isColumnEmpty(int col) {
                bool * column = valid + col * ALIEN_ROWS;
                for (int i = 0; i < ALIEN_ROWS; ++i)
                    if (*(column++))
                        return false;
                return true;
            }

            void drawOn(Bitmap<ColorRGB> & b) {
                bool * v = valid;
                for (int col = 0; col < ALIEN_COLS; ++col) {
                    for (int row = 0; row < ALIEN_ROWS; ++row) {
                        if (*(v++)) {
                            b.fill(color::Red, Rect::XYWH(x + col * 20 + 2, y - row * 20 + 2 - 20, 16, 16));
                        }
                    }
                }
            }

            bool checkBullet(Point bullet) {
                if (bullet.x < x || bullet.x > x + 20 * ALIEN_COLS)
                    return false;
                if (bullet.y > y || bullet.y < y - 20 * ALIEN_ROWS)
                    return false;
                int bulletCol = (bullet.x - x) / 20;
                int bulletRow = (y - bullet.y) / 20;
                if (!isValid(bulletCol, bulletRow))
                    return false;
                remove(bulletCol, bulletRow);
                return true;
            }

            void advance() {
                x = x + speed;
                if (x + (freeLeft * 20) < 0) {
                    x = 0 - (freeLeft * 20);
                    speed = speed * -1;
                } else if (x + (ALIEN_COLS - freeRight) * 20 > 320) {
                    x = 320 - (ALIEN_COLS - freeRight) * 20;
                    speed = speed * -1;
                }
            }

        }; 

        Aliens aliens_;

        std::vector<Point> bullets_;
        std::vector<Point> alienBullets_;

        Music music_;
        ToneGenerator audio_;

        static constexpr uint8_t melody_[] = {
            NOTE_2(E4), 
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_2(C4), 
            NOTE_4(C4),
            REST_4,
            NOTE_4(D4),
            NOTE_4(D4),
            NOTE_4(E4),
            NOTE_4(F4),
            NOTE_4(E4), 
            NOTE_4(D4),
            NOTE_4(C4),
            REST_4,
            NOTE_4(E4), 
            NOTE_4(G4), 
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_4(C4),
            NOTE_4(B3),
            NOTE_4(A3),
            REST_8,
            NOTE_8(C4),
            NOTE_4(D4),
            NOTE_4(C4),
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_2L(C4),
            REST_4,
            NOTE_4(E4),
            NOTE_4(E4),
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_2(C4),
            NOTE_4(C4),
            NOTE_4(C4),
            NOTE_4(D4),
            NOTE_4(D4),
            NOTE_4(D4),
            NOTE_4(F4),
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_4(C4),
            REST_4,
            NOTE_4(E4),
            NOTE_4(G4),
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_4(C4),
            NOTE_4(B3),
            NOTE_4(A3),
            REST_8,
            NOTE_8(C4),
            NOTE_4(D4),
            NOTE_4(D4),
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_2L(C4),
            REST_4,

            NOTE_2(E4), 
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_2(C4), 
            NOTE_4(C4),
            REST_4,
            NOTE_4(D4),
            NOTE_4(D4),
            NOTE_4(E4),
            NOTE_4(F4),
            NOTE_4(E4), 
            NOTE_4(D4),
            NOTE_4(C4),
            REST_4,
            NOTE_4(E4), 
            NOTE_4(G4), 
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_4(C4),
            NOTE_4(B3),
            NOTE_4(A3),
            REST_8,
            NOTE_8(C4),
            NOTE_4(D4),
            NOTE_4(C4),
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE(C4),
            
            REST,
        };

    }; // rckid::GalaxyInvaders

} // namespace rckid