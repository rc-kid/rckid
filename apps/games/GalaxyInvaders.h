#pragma once

#include <algorithm>

#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/graphics/sprite.h>
#include <rckid/ui/header.h>
#include <rckid/ui/alert.h>

namespace rckid {

    /** SpaceInvaders-like simple game. 
     
        Controllable ship.
     */
    class GalaxyInvaders : public GraphicsApp<Canvas<ColorRGB>> {
    public:

        static void run() {
            GalaxyInvaders g;
            g.resetGame();
            g.loop();
        }

    protected:

        GalaxyInvaders(): GraphicsApp{Canvas<Color>{320, 240}} {}

        void update() override {
            // handle back button
            GraphicsApp::update();
            bool moving = false;
            if (btnDown(Btn::Left)) {
                if (shipSpeed_ >= 0)
                    shipSpeed_ = -1;
                else if (shipSpeed_ > -10)
                    shipSpeed_ = shipSpeed_ * FixedInt{1, 0x20};
                shipX_ = shipX_ + shipSpeed_;
                if (shipX_ < 0)
                    shipX_ = 0;
                moving = true;
            }
            if (btnDown(Btn::Right)) {
                if (shipSpeed_ <= 0)
                    shipSpeed_ = 1;
                else if (shipSpeed_ < 10)
                    shipSpeed_ = shipSpeed_ * FixedInt{1, 0x20};
                shipX_ = shipX_ + shipSpeed_;
                if (shipX_ > 296)
                    shipX_ = 296;
                moving = true;
            }
            // fire
            if (btnPressed(Btn::A)) {
                bullets_.push_back(Point{shipX_ + 12, 200});
            }
            if (!moving)
                shipSpeed_ = 0;
            advanceBullets();
        }

        void draw() override {
            g_.fill();
            drawBullets();
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

        void drawBullets() {
            for (auto & pos : bullets_)
                g_.fill(color::White, Rect::XYWH(pos.x - 2, pos.y - 5, 4, 10));
        }


        void advanceBullets() {
            // advance bullets and check those that hit aliens
            for (auto & b: bullets_) {
                b.y -= 2;
                // TODO check if the bullet hits an alien
                if (aliens_.checkBullet(b)) {
                    b.y = -100;
                    rumbleNudge();
                }
            }
            // remove any off screen bullets
            bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(), [](Point const & p) {
                 return p.y < 0; 
            }), bullets_.end());
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
            FixedInt speed;
            int y;
            bool valid[ALIEN_COLS * ALIEN_ROWS];
            int freeLeft = 0;
            int freeRight = 0;

            void reset() {
                for (uint32_t i = 0; i < ALIEN_COLS * ALIEN_ROWS; ++i)
                    valid[i] = true;
                x = 30;
                y = 100;
            }

            bool isValid(int col, int row) {
                return valid[row + col * ALIEN_ROWS];
            }

            void remove(int col, int row) {
                valid[row + col * ALIEN_ROWS] = false;
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

        }; 

        Aliens aliens_;


        std::vector<Point> bullets_;

    }; // rckid::GalaxyInvaders


} // namespace rckid