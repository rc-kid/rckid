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
#include <rckid/assets/fonts/HemiHead32.h>
#include <rckid/assets/fonts/HemiHead64.h>
#include <rckid/assets/glyphs.h>
#include <rckid/assets/icons24.h>
#include <rckid/assets/icons16.h>

#include <rckid/ui/halloffame.h>
#include <rckid/filesystem.h>

namespace rckid {

    /** SpaceInvaders-like simple game. 

     */
    class GalaxyInvaders : public GraphicsApp<Canvas<ColorRGB>> {
    public:

        static void run() {
            GalaxyInvaders g;
            g.loop();
        }

    protected:

        static constexpr int ALIEN_COLS = 8;
        static constexpr int ALIEN_ROWS = 5;
        static constexpr uint8_t ALIEN_INVALID = 0xff;
        static constexpr unsigned NUM_STARS = 80;

        static constexpr int32_t SHIP_WIDTH = 24;
        static constexpr int32_t SHIP_HEIGHT = 24;
        static constexpr int32_t SHIP_Y = 195;

        GalaxyInvaders(): 
            GraphicsApp{ARENA(Canvas<Color>{320, 240})} /*, 
            // TODO for some resons, the commented code initialization does not work, while the one below does. Not sure why. a bug in my code? stack issues? - it only does not work on HW, fantasy works ok
            ship_{Bitmap<ColorRGB>::fromImage(PNG::fromBuffer(assets::icons24::space))},
            alienShips_{
                Bitmap<ColorRGB>::fromImage(PNG::fromBuffer(assets::icons16::alien_2)),
                Bitmap<ColorRGB>::fromImage(PNG::fromBuffer(assets::icons16::alien_1)),
                Bitmap<ColorRGB>::fromImage(PNG::fromBuffer(assets::icons16::alien)),
            } */ {
            alienShips_[0].loadImage(PNG::fromBuffer(assets::icons16::alien_2));
            alienShips_[1].loadImage(PNG::fromBuffer(assets::icons16::alien_1));
            alienShips_[2].loadImage(PNG::fromBuffer(assets::icons16::alien));
            ship_.loadImage(PNG::fromBuffer(assets::icons24::space));
            audio_[1].setEnvelope(100, 50, 80, 250);
            initializeStars();
            using namespace filesystem;
            FileRead f = fileRead("galaxyInvaders.hof", Drive::Cartridge);
            if (f.good()) {
                hof_.deserializeFrom(f);
            } else {
                hof_.add("Lightning", 10000);
                hof_.add("Mater", 1000);
                hof_.add("Doc Hudson", 100);
                hof_.add("Lemon", 1);
            }
            hof_.setTitleFont(assets::font::HemiHead32::font);
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
            updateStars();
            if (mode_ == Mode::Intro) {
                if (btnPressed(Btn::A))
                    resetGame();
            } else {
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
                int speedX = joystickX(-10, 10);
                if (speedX != 0)
                    shipX_ = (shipX_ + speedX).clamp(0, 296);
                int speedY = -joystickY(-10, 10);
                if (speedY != 0)
                    shipY_ = (shipY_ + speedY).clamp(0, SHIP_Y);
                // fire
                if (btnPressed(Btn::A)) {
                    bullets_.push_back(FixedPoint{shipX_ + 12, shipY_});
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
                if (advanceAlienBullets() || checkAlienPosition()) {
                    mode_ = Mode::Killed;
                    spawn_ = 120;
                    // ouch
                    audio_[1].setFrequency(50, 500);
                    msgText_ = "Kaboom!";
                } else {
                    if (aliens_.active == 0)
                        nextLevel();
                }
                if (btnPressed(Btn::B)) {
                    mode_ = Mode::Intro;
                    btnPressedClear(Btn::B);
                    shouldSaveHighScore();
                }
            }
            // handle back button
            GraphicsApp::update();
        }

        void draw() override {
            NewArenaScope _{};
            g_.fill();
            drawStars();
            if (mode_ == Mode::Intro) {
                hof_.drawOn(g_);
            } else {
                // draw lives
                for (uint32_t i = 0; i < lives_; ++i)
                    g_.text(i * 22, 216, assets::font::Symbols16::font, color::Red) << assets::glyph::SolidHeart;
                std::string score{STR(score_)};
                g_.text(160 - assets::font::OpenDyslexic24::font.textWidth(score) / 2, 218, assets::font::OpenDyslexic24::font, color::LightGray) << score;
                //std::string score{STR(aliens_.active)};
                //g_.text(160 - assets::font::OpenDyslexic24::font.textWidth(score) / 2, 216, assets::font::OpenDyslexic24::font, color::LightGray) << aliens_.active;
                std::string lvl(STR("Lvl: " << level_));
                g_.text(320 - assets::font::OpenDyslexic24::font.textWidth(lvl), 218, assets::font::OpenDyslexic24::font, color::LightGray) << lvl;
                // draw aliens
                aliens_.drawOn(g_, alienShips_);
                // draw the player depending on the spawn state
                if (mode_ == Mode::Respawn || mode_ == Mode::Killed) {
                    Font const & f = assets::font::HemiHead64::font;
                    g_.textRainbow(160 - f.textWidth(msgText_.c_str()) / 2, 80, f, msgHue_, 1024) << msgText_;
                    msgHue_ += 1024;
                    if ((spawn_ / 10) & 1)
                        return; 
                }
                drawBullets(bullets_, color::White);
                drawBullets(alienBullets_, color::Red);
                drawSpaceship();
            }
        }

        void resetGame() {
            score_ = 0;
            level_ = 0;
            lives_ = 3;
            nextLevel();
            msgText_ = "Get Ready!";
        }

        void respawnPlayer() {
            shipX_ = 160 - SHIP_WIDTH / 2;
            shipY_ = SHIP_Y;
            alienBullets_.clear();
            bullets_.clear();
            mode_ = Mode::Respawn;
            spawn_ = 120;
            msgText_ = "Kaboom!";
        }

        void nextLevel() {
            ++level_;
            shipX_ = 160 - SHIP_WIDTH / 2;
            shipY_ = SHIP_Y;
            aliens_.reset();
            bullets_.clear();
            alienBullets_.clear();
            mode_ = Mode::Respawn;
            spawn_ = 120;
            if (level_ == 1) {
                    alienFireProm_ = 0; 
                    aliens_.speed = FixedInt{0, 0x4};
                    aliens_.gravity = 0;
                    alienBulletSpeed_ = FixedInt{0, 0x2};
            } else if (level_ <= 42 ) {
                alienFireProm_ = level_;
                aliens_.speed = FixedInt{0, 0x8};
                aliens_.gravity = FixedInt{0, static_cast<uint8_t>(4 + level_ / 4)};
                alienBulletSpeed_ = FixedInt{0, static_cast<uint8_t>(2 + level_ / 4)};
            } else {
                alienFireProm_ = 20;
                aliens_.speed = FixedInt{1};
                aliens_.gravity = FixedInt{1};
                alienBulletSpeed_ = 2;
            }
            msgText_ = "Level Up!";
        }

        bool shouldSaveHighScore() {
            using namespace filesystem;
            if (hof_.isHighEnough(score_)) {
                auto name = runModal<TextInput>();
                if (name) {
                    hof_.add(name.value(), score_);
                    // update the hall of fame
                    FileWrite f = fileWrite("galaxyInvaders.hof", Drive::Cartridge);
                    hof_.serializeTo(f);
                }
                return true;
            } else {
                return false;
            }
        }

        void gameOver() {
            runModal<Alert>("GAME OVER", "Press A to continue...", ColorRGB{32, 0, 0});
            shouldSaveHighScore();
            resetGame();
        }

        void drawSpaceship() {
            g_.blit(Point{shipX_.round(), shipY_.round()}, ship_);
            //g_.fill(color::Green, Rect::XYWH(shipX_.round(), SHIP_Y, SHIP_WIDTH, SHIP_HEIGHT));
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
                if (b.y >= shipY_ && b.y <= shipY_ + SHIP_HEIGHT)
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

        /** Checks if any of the aliens overlaps with player, or if they have descended too low, in which case the player can't kill then anymore and is dead anyways.
         */
        bool checkAlienPosition() {
            return aliens_.checkPosition(shipX_, shipY_);
        }

        class Aliens {
        public:
            FixedInt x;
            //FixedInt speed{0, 0x8};
            FixedInt y;
            FixedInt speed;
            uint8_t valid[ALIEN_COLS * ALIEN_ROWS];
            int freeLeft = 0;
            int freeRight = 0;
            uint32_t active = ALIEN_COLS * ALIEN_ROWS;
            FixedInt gravity;


            void reset() {
                for (uint32_t i = 0; i < ALIEN_COLS * ALIEN_ROWS; ++i)
                    valid[i] = random() % 3;
                x = 160 - ALIEN_COLS * 10;
                y = 100;
                freeLeft = 0;
                freeRight = 0;
                active = ALIEN_COLS * ALIEN_ROWS;
            }

            bool isValid(int col, int row) {
                if (col < 0 || col >= ALIEN_COLS)
                    return false;
                if (row < 0 || row >= ALIEN_ROWS)
                    return false;
                return valid[row + col * ALIEN_ROWS] != ALIEN_INVALID;
            }

            void remove(int col, int row) {
                ASSERT(isValid(col, row));
                valid[row + col * ALIEN_ROWS] = ALIEN_INVALID;
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
                uint8_t * column = valid + col * ALIEN_ROWS;
                for (int i = 0; i < ALIEN_ROWS; ++i)
                    if (*(column++) != ALIEN_INVALID)
                        return false;
                return true;
            }

            void drawOn(Bitmap<ColorRGB> & b, Bitmap<ColorRGB> const * ships) {
                uint8_t * v = valid;
                for (int col = 0; col < ALIEN_COLS; ++col) {
                    for (int row = 0; row < ALIEN_ROWS; ++row) {
                        if (*v != ALIEN_INVALID) {
                            int dx = 0; //static_cast<int>(random() % 3) - 1;
                            int dy = 0; //static_cast<int>(random() % 3) - 1;
                            b.blit(Point{(x + col * 20 + 2).round() + dx, (y - row * 20 + 2 - 20).round() + dy}, ships[*v]);
                            
                        }
                        ++v;
                    }
                }
            }

            bool checkPosition(FixedInt px, FixedInt py) {
                uint8_t * v = valid;
                for (int col = 0; col < ALIEN_COLS; ++col) {
                    for (int row = 0; row < ALIEN_ROWS; ++row) {
                        if (*v != ALIEN_INVALID) {
                            FixedInt yy = y - row * 20 - 20;
                            // if the alien is below lowest player position
                            if (yy > 195 - 16)
                                return true;
                            // if player is in different part of screen row-wise, no need to check further
                            if (yy >= py - 16 && yy <= py + 24) {
                                FixedInt xx = x + col * 20 + 2;
                                if (xx > px - 16 && xx < px + 24)
                                    return true;
                            }
                        }
                        ++v;
                    }
                }
                return false;
            }

            bool checkBullet(FixedPoint bullet) {
                int bx = bullet.x.clip();
                int by = bullet.y.clip();
                if (bx < x || bx > x + 20 * ALIEN_COLS)
                    return false;
                if (by > y || by < y - 20 * ALIEN_ROWS)
                    return false;
                int bulletCol = (bx - x).clip() / 20;
                int bulletRow = (y - by).clip() / 20;
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
                    y += gravity;
                } else if (x + (ALIEN_COLS - freeRight) * 20 > 320) {
                    x = 320 - (ALIEN_COLS - freeRight) * 20;
                    speed = speed * -1;
                    y += gravity;
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
                uint8_t * column = valid + col * ALIEN_ROWS;
                int row = 0;
                while (column[row] == ALIEN_INVALID)
                    ++row;
                return FixedPoint{x.round() + col * 20 + 10, y - row * 20};
            }

        }; 

        void drawStars() {
            for (unsigned i = 0; i < NUM_STARS; ++i) {
                uint8_t b = starBrightness_[i];
                g_.setPixelAt(stars_[i].x.round(), stars_[i].y.round(),  ColorRGB{b, b, b});
            }
        }

        void initializeStars() {
            for (unsigned i = 0; i < NUM_STARS; ++i) {
                stars_[i].y = random() % 240;
                stars_[i].x = random() % 320;
                starSpeed_[i] = FixedInt{static_cast<int>(1 + random() % 3), static_cast<uint8_t>(random() % 16)};
                starBrightness_[i] = 128 + random() % 128;
            }
        }

        void updateStars() {
            for (unsigned i = 0; i < NUM_STARS; ++i) {
                stars_[i].y += starSpeed_[i];
                if (stars_[i].y > 240) {
                    stars_[i].y = 0;
                    stars_[i].x = random() % 320;
                    starSpeed_[i] = FixedInt{static_cast<int>(1 + random() % 3), static_cast<uint8_t>(random() % 16)};
                    starBrightness_[i] = 128 + random() % 128;
                }
            }
        }

        enum class Mode {
            Intro, // intro mode with high score, stars & aliens
            Game, // active game
            Killed, // player ship is killed, will respawn or game over when countdown is done 
            Respawn, // player is respawn
        };

        Mode mode_ = Mode::Intro;

        uint32_t spawn_ = 0;
        Aliens aliens_;

        std::vector<FixedPoint> bullets_;
        std::vector<FixedPoint> alienBullets_;

        Bitmap<ColorRGB> ship_{24, 24};
        Bitmap<ColorRGB> alienShips_[3] = {
            Bitmap<ColorRGB>{16, 16},
            Bitmap<ColorRGB>{16, 16},
            Bitmap<ColorRGB>{16, 16},
        };

        Music music_;
        ToneGenerator audio_;

        uint32_t level_;
        uint32_t score_;
        uint32_t lives_;
        // promile per tick for an alien to shoot (anything above 50 is crazy)
        uint32_t alienFireProm_;
        FixedInt alienBulletSpeed_;
        std::string msgText_ = "Get Ready!"; 
        uint16_t msgHue_ = 0;

        FixedInt shipX_;
        FixedInt shipY_ = SHIP_Y;

        // hall of fame screen
        HallOfFame hof_;



        FixedPoint stars_[NUM_STARS];
        FixedInt starSpeed_[NUM_STARS];
        uint8_t starBrightness_[NUM_STARS];


    }; // rckid::GalaxyInvaders

} // namespace rckid