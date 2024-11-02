#pragma once

#include <algorithm>

#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/graphics/sprite.h>
#include <rckid/ui/header.h>
#include <rckid/ui/alert.h>
#include <rckid/audio/tone.h>
#include <rckid/audio/music.h>
#include <rckid/assets/fonts/Symbols16.h>
#include <rckid/assets/fonts/OpenDyslexic24.h>
#include <rckid/assets/glyphs.h>

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

        static constexpr int32_t SHIP_WIDTH = 24;
        static constexpr int32_t SHIP_HEIGHT = 24;
        static constexpr int32_t SHIP_Y = 195;

        GalaxyInvaders(): GraphicsApp{Canvas<Color>{320, 240}} {
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
            // don't do anything if we are spawning, just decrease the spawn counter
            if (spawn_ > 0) {
                if (--spawn_ == 0) {
                    if (mode_ == Mode::Killed) {
                        if (--lives_ > 0)
                            respawnPlayer();
                        else 
                            gameOver();
                    } else {
                        mode_ = Mode::Game;
                    }
                }
                return;
            }
            // ship's position
            int speed = joystickX(-10, 10);
            if (speed != 0)
                shipX_ = (shipX_ + speed).clamp(0, 296);
            // fire
            if (btnPressed(Btn::A)) {
                bullets_.push_back(FixedPoint{shipX_ + 12, 200});
                audio_[0].setFrequency(440, 100);
            }
            // should aliens fire?
            if (random() % 1000 < alienFireProm_) {
                alienBullets_.push_back(aliens_.shoot());
                audio_[0].setFrequency(800, 100);
            }
            // update the position if aliens and calculate any bullets
            aliens_.advance();
            advanceBullets();
            if (advanceAlienBullets()) {
                mode_ = Mode::Killed;
                spawn_ = 120;
                // ouch
                audio_[1].setFrequency(50, 500);
            } else {
                if (aliens_.active == 0)
                    nextLevel();
            }
        }

        void draw() override {
            g_.fill();
            // draw lives
            for (uint32_t i = 0; i < lives_; ++i)
                g_.text(i * 22, 216, assets::font::Symbols16::font, color::Red) << assets::glyph::SolidHeart;
            //std::string score{STR(score_)};
            //g_.text(160 - assets::font::OpenDyslexic24::font.textWidth(score) / 2, 216, assets::font::OpenDyslexic24::font, color::LightGray) << score;
            std::string score{STR(aliens_.active)};
            g_.text(160 - assets::font::OpenDyslexic24::font.textWidth(score) / 2, 216, assets::font::OpenDyslexic24::font, color::LightGray) << aliens_.active;
            std::string lvl(STR("Lvl: " << level_));
            g_.text(320 - assets::font::OpenDyslexic24::font.textWidth(lvl), 216, assets::font::OpenDyslexic24::font, color::LightGray) << lvl;
            // draw aliens
            aliens_.drawOn(g_);
            // draw the player depending on the spawn state
            if (mode_ == Mode::Respawn || mode_ == Mode::Killed)
                if ((spawn_ / 10) & 1)
                    return; 
            drawBullets(bullets_, color::White);
            drawBullets(alienBullets_, color::Red);
            drawSpaceship();
        }

        void resetGame() {
            score_ = 0;
            level_ = 0;
            lives_ = 3;
            nextLevel();
        }

        void respawnPlayer() {
            shipX_ = 160 - SHIP_WIDTH / 2;
            alienBullets_.clear();
            bullets_.clear();
            mode_ = Mode::Respawn;
            spawn_ = 120;
        }

        void nextLevel() {
            ++level_;
            shipX_ = 160 - SHIP_WIDTH / 2;
            aliens_.reset();
            bullets_.clear();
            alienBullets_.clear();
            mode_ = Mode::Respawn;
            spawn_ = 120;
            switch (level_) {
                case 1:
                    alienFireProm_ = 0; 
                    aliens_.speed = FixedInt{0, 0x4};
                    alienBulletSpeed_ = FixedInt{0, 0x8};
                    break;                    
                case 2:
                    alienFireProm_ = 1;
                    aliens_.speed = FixedInt{0, 0x8};
                    break;
                default:
                    alienFireProm_ = 50;
                    aliens_.speed = FixedInt{1};
            }
        }

        void gameOver() {
            runModal<Alert>("GAME OVER", "Press A to continue...", ColorRGB{32, 0, 0});
            resetGame();
        }

        void drawSpaceship() {
            g_.fill(color::Green, Rect::XYWH(shipX_.round(), SHIP_Y, SHIP_WIDTH, SHIP_HEIGHT));
        }

        void drawBullets(std::vector<FixedPoint> const & bullets, Color color) {
            for (auto & pos : bullets)
                g_.fill(color, Rect::XYWH((pos.x - 2).round(), (pos.y - 5).round(), 4, 10));
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
                    score_ += level_;
                }
            }
            // remove any off screen bullets
            bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(), [](FixedPoint const & p) {
                 return p.y < 0; 
            }), bullets_.end());
        }

        bool advanceAlienBullets() {
            // advance bullets and check those that hit aliens
            for (auto & b: alienBullets_) {
                b.y += alienBulletSpeed_;
                // check if the bullet hits player
                if (b.y >= SHIP_Y && b.y <= SHIP_Y + SHIP_HEIGHT)
                    if (b.x >= shipX_ && b.x <= shipX_ + SHIP_WIDTH)
                        // we'll be respawning the player which clears all bullets so no need to deal the bullet 
                        return true;
            }
            // remove any off screen bullets
            alienBullets_.erase(std::remove_if(alienBullets_.begin(), alienBullets_.end(), [](FixedPoint const & p) {
                 return p.y > 220; 
            }), alienBullets_.end());
            return false;
        }

        uint32_t level_;
        uint32_t score_;
        uint32_t lives_;
        // promile per tick for an alien to shoot (anything above 50 is crazy)
        uint32_t alienFireProm_;
        FixedInt alienBulletSpeed_;

        FixedInt shipX_;

        static constexpr int ALIEN_COLS = 8;
        static constexpr int ALIEN_ROWS = 5;

        class Aliens {
        public:
            FixedInt x;
            //FixedInt speed{0, 0x8};
            FixedInt y;
            FixedInt speed;
            bool valid[ALIEN_COLS * ALIEN_ROWS];
            int freeLeft = 0;
            int freeRight = 0;
            uint32_t active = ALIEN_COLS * ALIEN_ROWS;

            void reset() {
                for (uint32_t i = 0; i < ALIEN_COLS * ALIEN_ROWS; ++i)
                    valid[i] = true;
                x = 160 - ALIEN_COLS * 10;
                y = 100;
                freeLeft = 0;
                freeRight = 0;
                active = ALIEN_COLS * ALIEN_ROWS;
            }

            bool isValid(int col, int row) {
                ASSERT(col >= 0 && col < ALIEN_COLS);
                ASSERT(row >= 0 && row < ALIEN_ROWS);
                return valid[row + col * ALIEN_ROWS];
            }

            void remove(int col, int row) {
                ASSERT(isValid(col, row));
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
                --active;
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
                            b.fill(color::Red, Rect::XYWH((x + col * 20 + 2).round(), (y - row * 20 + 2 - 20).round(), 16, 16));
                        }
                    }
                }
            }

            bool checkBullet(FixedPoint bullet) {
                int bx = bullet.x.clip();
                int by = bullet.y.clip();
                if (bx < x || bx > x + 20 * ALIEN_COLS)
                    return false;
                if (by > y || by < y - 20 * ALIEN_ROWS)
                    return false;
                int bulletCol = (bx - x).round() / 20;
                int bulletRow = (y - by).round() / 20;
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

            /** Randomly selects a column from which the aliens will shoot and returns the bullet coordinates.
             */
            FixedPoint shoot() {
                uint32_t rnd = random() % ALIEN_COLS;
                int col = 0;
                while (true) {
                    if (!isColumnEmpty(col)) {
                        if (rnd == 0)
                            break;
                        --rnd;
                    }
                    col = (col + 1) % ALIEN_COLS;
                }
                bool * column = valid + col * ALIEN_ROWS;
                int row = 0;
                while (column[row] == false)
                    ++row;
                return FixedPoint{x.round() + col * 20 + 10, y - row * 20};
            }

        }; 

        enum class Mode {
            Game, // active game
            Killed, // player ship is killed, will respawn or game over when countdown is done 
            Respawn, // player is respawn
        };

        Mode mode_;

        uint32_t spawn_ = 0;
        Aliens aliens_;

        std::vector<FixedPoint> bullets_;
        std::vector<FixedPoint> alienBullets_;

        Music music_;
        ToneGenerator audio_;


    }; // rckid::GalaxyInvaders

} // namespace rckid