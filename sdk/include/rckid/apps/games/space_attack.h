#pragma once

#include <algorithm>
#include <vector>

#include <rckid/graphics/canvas.h>
#include <rckid/joystick.h>

#include <assets/games/space_attack.h>

#include <assets/Baloo64.h>
#include <assets/Iosevka24.h>
#include <assets/OpenDyslexic24.h>

namespace rckid {

    /** Space Attack, a space invaders like game.
     
        Shoot the descending aliens before they shoot you, or land on you. This is a port of the older GalaxyInvaders version I made, renamed so that it does not step on the original's name.
     */
    class SpaceAttack : public CanvasApp<void> {
    public:

        String name() const override { return "SpaceAttack"; }

        Capabilities capabilities() const override {
            return Capabilities {
                .canPersistState = true,
                .consumesBudget = true,
            };
        }

        static constexpr uint8_t VERSION = 1;

        SpaceAttack():
            CanvasApp<void>{},
            ship_{ImageSource{assets::games::space_attack::ship}},
            life_{ImageSource{assets::games::space_attack::life}},
            alienShips_{
                Bitmap{ImageSource{assets::games::space_attack::alien_2}},
                Bitmap{ImageSource{assets::games::space_attack::alien_1}},
                Bitmap{ImageSource{assets::games::space_attack::alien}},
            }
        {
            //audio_[1].setEnvelope(100, 50, 80, 250);
            initializeStars();
            resetGame();
            mode_ = Mode::Intro;
        }

    protected:

        void saveState(RandomWriteStream & into) const override {
            auto wr = into.binaryWriter();
            wr << VERSION << static_cast<uint8_t>(mode_);
            if (mode_ == Mode::Intro)
                return;
            wr  << level_
                << score_
                << lives_
                << spawn_
                << alienFireProm_
                << alienBulletSpeed_
                << shipX_
                << shipY_;
            aliens_.serializeTo(wr);
            writeBullets(wr, bullets_);
            writeBullets(wr, alienBullets_);
        }

        bool loadState(RandomReadStream & from) override {
            auto rd = from.binaryReader();
            if (read<uint8_t>(rd) != VERSION) {
                LOG(LL_WARN, "Unsupported save version, skipping");
                return false;
            }
            mode_ = static_cast<Mode>(read<uint8_t>(rd));
            if (mode_ == Mode::Intro)
                return true;
            rd  >> level_
                >> score_
                >> lives_
                >> spawn_
                >> alienFireProm_
                >> alienBulletSpeed_
                >> shipX_
                >> shipY_;
            aliens_.deserializeFrom(rd);
            readBullets(rd, bullets_);
            readBullets(rd, alienBullets_);
            return true;
        }

        void loop() override {
            CanvasApp<void>::loop();
            updateStars();
            if (mode_ == Mode::Intro) {
                if (btnPressed(Btn::A) || btnPressed(Btn::Start))
                    resetGame();
                if (btnPressed(Btn::B)) {
                    btnClear(Btn::B);
                    exit();
                }
                return;
            }
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
            joystick_.update();
            int32_t speedX = joystick_.x.value(-10, 10);
            if (speedX != 0)
                shipX_ = (shipX_ + speedX).clamp(0, 296);
            int32_t speedY = - joystick_.y.value(-10, 10);
            if (speedY != 0)
                shipY_ = (shipY_ + speedY).clamp(0, SHIP_Y);
            // fire
            if (btnPressed(Btn::A)) {
                bullets_.push_back(FixedPoint{shipX_ + 12, shipY_});
                //audio_[0].setFrequency(440, 100);
            }
            // should aliens fire?
            if (static_cast<uint32_t>(random() % 1000) < alienFireProm_) {
                alienBullets_.push_back(aliens_.shoot());
                //audio_[0].setFrequency(800, 100);
            }
            // update the position of aliens and calculate any bullets
            aliens_.advance();
            advanceBullets();
            if (advanceAlienBullets() || checkAlienPosition()) {
                mode_ = Mode::Killed;
                spawn_ = 120;
                // ouch
                //audio_[1].setFrequency(50, 500);
                msgText_ = "Kaboom!";
            } else {
                if (aliens_.active == 0)
                    nextLevel();
            }
            if (btnPressed(Btn::B)) {
                btnClear(Btn::B);
                mode_ = Mode::Intro;
                //shouldSaveHighScore();
            }
        }

        void render() override {
            canvas().fill(Color::Black());
            drawStars();
            if (mode_ == Mode::Intro) {
                drawIntro();
            } else {
                // draw lives
                for (uint32_t i = 0; i < lives_; ++i)
                    canvas().blit(Point{static_cast<Coord>(i) * 26, 214}, life_);
                Font f{assets::OpenDyslexic24};
                String score{STR(score_)};
                canvas().text(160 - f->textWidth(score) / 2, 218, f, LIGHT_GRAY) << score;
                String lvl{STR("Lvl: " << level_)};
                canvas().text(320 - f->textWidth(lvl), 218, f, LIGHT_GRAY) << lvl;
                // draw aliens
                aliens_.drawOn(canvas(), alienShips_);
                // draw the player depending on the spawn state
                if (mode_ == Mode::Respawn || mode_ == Mode::Killed) {
                    Font mf{assets::Baloo64};
                    canvas().textRainbow(160 - mf->textWidth(msgText_) / 2, 80, mf, msgHue_, 1024) << msgText_;
                    msgHue_ += 1024;
                    if ((spawn_ / 10) & 1) {
                        CanvasApp<void>::render();
                        return; 
                    }
                }
                drawBullets(bullets_, Color::White());
                drawBullets(alienBullets_, Color::Red());
                drawSpaceship();
            }
            CanvasApp<void>::render();
        }

    private:

        static constexpr Coord ALIEN_COLS = 8;
        static constexpr Coord ALIEN_ROWS = 5;
        static constexpr uint8_t ALIEN_INVALID = 0xff;
        static constexpr unsigned NUM_STARS = 80;

        static constexpr int32_t SHIP_WIDTH = 24;
        static constexpr int32_t SHIP_HEIGHT = 24;
        static constexpr int32_t SHIP_Y = 195;

        static constexpr char const * TITLE = "SPACE ATTACK";

        static inline Color const LIGHT_GRAY = Color::White().withBrightness(192);

        enum class Mode {
            Intro, // intro mode with the title & stars
            Game, // active game
            Killed, // player ship is killed, will respawn or game over when countdown is done 
            Respawn, // player is respawn
        };

        void drawIntro() {
            Font f{assets::Baloo64};
            Coord tw = f->textWidth(TITLE);
            canvas().textRainbow(160 - tw / 2, 40, f, startHue_, 1024) << TITLE;
            Font f2{assets::Iosevka24};
            tw = f2->textWidth("Press A to start");
            canvas().textRainbow(160 - tw / 2, 160, f2, startHue_, -1024) << "Press A to start";
            startHue_ += 128;
            // the hall of fame used to be shown here, alternating with the title. Left out until the SDK has the text input & the hall of fame widget again.
            //hof_.drawOn(canvas());
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
            joystick_.reset();
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
            joystick_.reset();
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

        /* The hall of fame is not ported yet - the new SDK has no hall of fame widget and the game over alert would need the text input as well. Kept here so that it can be revived when it is.

        bool shouldSaveHighScore() {
            if (hof_.isHighEnough(score_)) {
                auto name = App::run<TextDialog>("");
                if (name) {
                    hof_.add(name.value(), score_);
                    auto f = writeFile("spaceAttack.hof");
                    if (f != nullptr)
                        hof_.serializeTo(*f);
                }
                return true;
            } else {
                return false;
            }
        }
        */

        void gameOver() {
            //runModal<Alert>("GAME OVER", "Press A to continue...", ColorRGB{32, 0, 0});
            //shouldSaveHighScore();
            resetGame();
            mode_ = Mode::Intro;
        }

        void drawSpaceship() {
            canvas().blit(Point{shipX_.round(), shipY_.round()}, ship_);
        }

        void drawBullets(std::vector<FixedPoint> const & bullets, Color color) {
            for (auto & pos : bullets)
                canvas().fill(Rect::XYWH((pos.x - 2).round(), (pos.y - 5).round(), 4, 10), color);
        }

        void advanceBullets() {
            // advance bullets and check those that hit aliens
            for (auto & b: bullets_) {
                b.y -= 2;
                // check if the bullet hits an alien
                if (aliens_.checkBullet(b)) {
                    b.y = -100;
                    //audio_[1].setFrequency(50, 500);
                    rumbler::nudge();
                    score_ += level_;
                }
            }
            // remove any off screen bullets
            bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(), [](FixedPoint const & p) {
                 return p.y < 0; 
            }), bullets_.end());
        }

        bool advanceAlienBullets() {
            // advance bullets and check those that hit the player
            for (auto & b: alienBullets_) {
                b.y += alienBulletSpeed_;
                // check if the bullet hits player
                if (b.y >= shipY_ && b.y <= shipY_ + SHIP_HEIGHT)
                    if (b.x >= shipX_ && b.x <= shipX_ + SHIP_WIDTH)
                        // we'll be respawning the player which clears all bullets so no need to deal with the bullet 
                        return true;
            }
            // remove any off screen bullets
            alienBullets_.erase(std::remove_if(alienBullets_.begin(), alienBullets_.end(), [](FixedPoint const & p) {
                 return p.y > 220; 
            }), alienBullets_.end());
            return false;
        }

        /** Checks if any of the aliens overlaps with player, or if they have descended too low, in which case the player can't kill them anymore and is dead anyways.
         */
        bool checkAlienPosition() {
            return aliens_.checkPosition(shipX_, shipY_);
        }

        static void writeBullets(BinaryWriter & w, std::vector<FixedPoint> const & bullets) {
            w << static_cast<uint32_t>(bullets.size());
            for (auto const & b : bullets)
                w << b.x << b.y;
        }

        static void readBullets(BinaryReader & r, std::vector<FixedPoint> & bullets) {
            bullets.clear();
            uint32_t n = read<uint32_t>(r);
            for (; n > 0; --n) {
                FixedPoint b;
                r >> b.x >> b.y;
                bullets.push_back(b);
            }
        }

        class Aliens {
        public:
            FixedInt x;
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

            void serializeTo(BinaryWriter & w) const {
                w << x << y << speed << gravity
                  << static_cast<int32_t>(freeLeft)
                  << static_cast<int32_t>(freeRight)
                  << active
                  << wrapBuffer(valid, sizeof(valid));
            }

            void deserializeFrom(BinaryReader & r) {
                int32_t fl;
                int32_t fr;
                r >> x >> y >> speed >> gravity >> fl >> fr >> active >> wrapBuffer(valid, sizeof(valid));
                freeLeft = fl;
                freeRight = fr;
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

            void drawOn(Canvas & canvas, Bitmap * ships) {
                uint8_t * v = valid;
                for (int col = 0; col < ALIEN_COLS; ++col) {
                    for (int row = 0; row < ALIEN_ROWS; ++row) {
                        if (*v != ALIEN_INVALID) {
                            int dx = 0; //static_cast<int>(random() % 3) - 1;
                            int dy = 0; //static_cast<int>(random() % 3) - 1;
                            canvas.blit(Point{(x + col * 20 + 2).round() + dx, (y - row * 20 + 2 - 20).round() + dy}, ships[*v]);
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

        }; // SpaceAttack::Aliens

        void drawStars() {
            for (unsigned i = 0; i < NUM_STARS; ++i) {
                uint8_t b = starBrightness_[i];
                canvas().setPixel(stars_[i].x.round(), stars_[i].y.round(), Color::RGB(b, b, b));
            }
        }

        void initializeStars() {
            for (unsigned i = 0; i < NUM_STARS; ++i) {
                stars_[i].y = static_cast<int>(random() % 240);
                stars_[i].x = static_cast<int>(random() % 320);
                starSpeed_[i] = FixedInt{static_cast<int>(1 + random() % 3), static_cast<uint32_t>(random() % 16)};
                starBrightness_[i] = static_cast<uint8_t>(128 + random() % 128);
            }
        }

        void updateStars() {
            for (unsigned i = 0; i < NUM_STARS; ++i) {
                stars_[i].y += starSpeed_[i];
                if (stars_[i].y > 240) {
                    stars_[i].y = 0;
                    stars_[i].x = static_cast<int>(random() % 320);
                    starSpeed_[i] = FixedInt{static_cast<int>(1 + random() % 3), static_cast<uint32_t>(random() % 16)};
                    starBrightness_[i] = static_cast<uint8_t>(128 + random() % 128);
                }
            }
        }

        Mode mode_ = Mode::Intro;

        uint32_t spawn_ = 0;
        Aliens aliens_;

        std::vector<FixedPoint> bullets_;
        std::vector<FixedPoint> alienBullets_;

        Bitmap ship_;
        Bitmap life_;
        Bitmap alienShips_[3];

        //Music music_;
        //ToneGenerator audio_;

        Joystick joystick_;

        uint32_t level_ = 0;
        uint32_t score_ = 0;
        uint32_t lives_ = 3;
        // promile per tick for an alien to shoot (anything above 50 is crazy)
        uint32_t alienFireProm_ = 0;
        FixedInt alienBulletSpeed_;
        String msgText_ = "Get Ready!"; 
        uint16_t msgHue_ = 0;
        // hue for the intro title
        uint16_t startHue_ = 0;

        FixedInt shipX_;
        FixedInt shipY_ = SHIP_Y;

        // hall of fame screen
        //HallOfFame hof_;

        FixedPoint stars_[NUM_STARS];
        FixedInt starSpeed_[NUM_STARS];
        uint8_t starBrightness_[NUM_STARS];

    }; // rckid::SpaceAttack

} // namespace rckid
