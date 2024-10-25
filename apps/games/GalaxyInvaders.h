#pragma once

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
            if (!moving)
                shipSpeed_ = 0;
        }

        void draw() override {
            g_.fill();
            drawSpaceship();
            drawAliens();
        }

        void resetGame() {
            score_ = 0;
            shipX_ = 160;
            aliensY_ = 25;
            aliensX_ = 30;
        }

        void drawSpaceship() {
            g_.fill(color::Green, Rect::XYWH(shipX_, 200, 24, 24));
        }

        void drawBullets() {

        }

        void drawAliens() {
            int y = aliensY_;
            for (int r = 0; r < 5; ++r) {
                int x = aliensX_;
                for (int c = 0; c < 8; ++c) {
                    g_.fill(color::Red, Rect::XYWH(x, y, 16, 16));    
                    x += 20;
                }
                y += 20;
            }
        }

        uint32_t score_;

        FixedInt shipX_;
        FixedInt shipSpeed_;
        int32_t aliensX_;
        int32_t aliensY_;
        bool aliens_[8 * 5];


        std::vector<Point> bullets_;

    }; // rckid::GalaxyInvaders


} // namespace rckid