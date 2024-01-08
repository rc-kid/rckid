#pragma once

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/graphics/png.h"
#include "rckid/graphics/animation.h"

namespace rckid {

    /** A simple sliding puzzle game, silimar to FifteenPuzzle. 
     
        More of a demonstration of what can be done. 

        TODO:

          - support different picture sizes
          - support multiple tile sizes
          - animate transitions
          - dpad wiring seems counterintuitive atm
          - add accelerometer as input

     */
    class SlidingPuzzle : public App<FrameBuffer<ColorRGB>> {
    public:

    protected:

        void onFocus(BaseApp * previous) override {
            App::onFocus(previous);
            PNG png = PNG::fromBuffer(defaultImage_, sizeof(defaultImage_));
            png.decode([&](ColorRGB * line, int lineNum, int lineWidth){
                Renderer & r = renderer();
                for (int i = 0; i < lineWidth; ++i)
                    r.setPixelAt(i, lineNum, line[i]);
            });
            shuffle_ = 0;
            holeX_ = -1;
            holeY_ = -1;
            //Renderer & r = renderer();
            //r.setBg(Color::RGB(128, 128, 128));
            //r.fill(Rect::XYWH(320 - 40, 240 - 40, 40, 40));
        }

        void update() override {
            if (pressed(Btn::Start))
                shuffle_ = 20;
            if (dir_ == Btn::Home) {
                Renderer & r = renderer();
                if (pressed(Btn::Left) && (holeX_ < MAX_X) && (holeX_ >= 0)) {
                    holeX_ += 1;
                    dir_ = Btn::Left;
                } else if (pressed(Btn::Right) && (holeX_ > 0)) {
                    holeX_ -= 1;
                    dir_ = Btn::Right;
                } else if (pressed(Btn::Up) && (holeY_ < MAX_Y) && (holeY_ >= 0)) {
                    holeY_ += 1;
                    dir_ = Btn::Up;
                } else if (pressed(Btn::Down) && (holeY_ > 0)) {
                    holeY_ -= 1;
                    dir_ = Btn::Down;
                } else {
                    return;
                }
                tmp_.draw(r, 0, 0, tileRect(holeX_, holeY_));
                a_.start();
            }
        }

        void draw() override {
            Renderer & r = renderer();
            a_.update();
            if (shuffle_ > 0) {
                int x1 = get_rand_32() % (320 / TILE_WIDTH);
                int y1 = get_rand_32() % (240 / TILE_HEIGHT);
                int x2 = get_rand_32() % (320 / TILE_WIDTH);
                int y2 = get_rand_32() % (240 / TILE_HEIGHT);
                swapTiles(x1, y1, x2, y2);
                if (--shuffle_ == 0) {
                    holeX_ = MAX_X;
                    holeY_ = MAX_Y;
                    hole_.draw(r, 0, 0, tileRect(holeX_, holeY_));
                    r.setBg(ColorRGB::RGB(128, 128, 128));
                    r.fill(tileRect(holeX_, holeY_));
                }
            }
            switch (dir_) {
                case Btn::Left:
                    r.fill(tileRect(holeX_, holeY_));
                    r.draw(tmp_, tilePoint(holeX_, holeY_) - Point{a_.interpolate(0, TILE_WIDTH), 0});
                    break;
                case Btn::Right:
                    r.fill(tileRect(holeX_, holeY_));
                    r.draw(tmp_, tilePoint(holeX_, holeY_) + Point{a_.interpolate(0, TILE_WIDTH), 0});
                    break;
                case Btn::Up:
                    r.fill(tileRect(holeX_, holeY_));
                    r.draw(tmp_, tilePoint(holeX_, holeY_) - Point{0, a_.interpolate(0, TILE_HEIGHT)});
                    break;
                case Btn::Down: 
                    r.fill(tileRect(holeX_, holeY_));
                    r.draw(tmp_, tilePoint(holeX_, holeY_) + Point{0, a_.interpolate(0, TILE_HEIGHT)});
                    break;
                default:
                    break; // nothing to do for other controls
            }
            if (!a_.running())
                dir_ = Btn::Home;
        }

        void swapTiles(int x1, int y1, int x2, int y2) {
            Renderer & r = renderer();
            Rect t1 = tileRect(x1, y1);
            Rect t2 = tileRect(x2, y2);
            tmp_.draw(r, 0, 0, t1);
            r.draw(r, tilePoint(x1, y1), t2);
            r.draw(tmp_, tilePoint(x2, y2));
        }

    private:

        static constexpr int TILE_WIDTH = 80;
        static constexpr int TILE_HEIGHT = 80;

        static constexpr int MAX_X = 320 / TILE_WIDTH - 1;
        static constexpr int MAX_Y = 240 / TILE_HEIGHT - 1;

        Rect tileRect(int x, int y) {
            return Rect::XYWH(x * TILE_WIDTH, y * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT);
        }

        Point tilePoint(int x, int y) {
            return Point{ x * TILE_WIDTH, y * TILE_HEIGHT};
        }

        Canvas<ColorRGB> hole_{TILE_WIDTH,TILE_HEIGHT};
        Canvas<ColorRGB> tmp_{TILE_WIDTH,TILE_HEIGHT};

        uint8_t * tileMap_ = nullptr;

        int holeX_;
        int holeY_;

        size_t shuffle_ = 0;

        Btn dir_ = Btn::Home;

        Animation a_{500};

        /** PNG image for the game. 
         
           from https://images6.fanpop.com/image/photos/43100000/Disney-Princesses-disney-princess-43157173-1500-1080.jpg 
         */
        static constexpr uint8_t defaultImage_[] = {
#include "SlidingPuzzle16.png.data"
        }; 

    }; 

} // namespace rckid