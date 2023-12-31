#pragma once

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/graphics/png.h"

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
    class SlidingPuzzle : public App<FrameBuffer> {
    public:

    protected:

        void onFocus(BaseApp * previous) override {
            App::onFocus(previous);
            PNG png = PNG::fromBuffer(defaultImage_, sizeof(defaultImage_));
            png.decode([&](Color * line, int lineNum, int lineWidth){
                Renderer & r = renderer();
                for (int i = 0; i < lineWidth; ++i)
                    r.pixel(i, lineNum, line[i]);
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
                shuffle_ = 100;
            if (dir_ == Btn::Home) {
                if (pressed(Btn::Left)) {
                    dir_ = Btn::Left;
                } else if (pressed(Btn::Right)) {
                    dir_ = Btn::Right;
                } else if (pressed(Btn::Up)) {
                    dir_ = Btn::Up;
                } else if (pressed(Btn::Down)) {
                    dir_ = Btn::Down;
                }
            }
        }

        void draw() override {
            if (shuffle_ > 0) {
                int x1 = get_rand_32() % (320 / TILE_WIDTH);
                int y1 = get_rand_32() % (240 / TILE_HEIGHT);
                int x2 = get_rand_32() % (320 / TILE_WIDTH);
                int y2 = get_rand_32() % (240 / TILE_HEIGHT);
                swapTiles(x1, y1, x2, y2);
                if (--shuffle_ == 0) {
                    Renderer & r = renderer();
                    holeX_ = MAX_X;
                    holeY_ = MAX_Y;
                    hole_.draw(r, 0, 0, tileRect(holeX_, holeY_));
                    r.setBg(Color::RGB(128, 128, 128));
                    r.fill(tileRect(holeX_, holeY_));
                }

            }
            switch (dir_) {
                case Btn::Left:
                    if (holeX_ < MAX_X && holeX_ >= 0) {
                        swapTiles(holeX_, holeY_, holeX_ + 1, holeY_);
                        holeX_ += 1;
                    }
                    break;
                case Btn::Right:
                    if (holeX_ > 0) {
                        swapTiles(holeX_, holeY_, holeX_ - 1, holeY_);
                        holeX_ -= 1;
                    }
                    break;
                case Btn::Up:
                    if (holeY_ < MAX_Y && holeY_ >= 0) {
                        swapTiles(holeX_, holeY_, holeX_, holeY_ + 1);
                        holeY_ += 1;
                    }
                    break;
                case Btn::Down: 
                    if (holeY_ > 0) {
                        swapTiles(holeX_, holeY_, holeX_, holeY_ - 1);
                        holeY_ -= 1;
                    }
                    break;
                default:
                    break; // nothing to do for other controls
            }
            dir_ = Btn::Home;
        }

        void shuffle() {
            for (int i = 0; i < 100; ++i) {
                int x1 = get_rand_32() % (320 / TILE_WIDTH);
                int y1 = get_rand_32() % (240 / TILE_HEIGHT);
                int x2 = get_rand_32() % (320 / TILE_WIDTH);
                int y2 = get_rand_32() % (240 / TILE_HEIGHT);
                swapTiles(x1, y1, x2, y2);
            }
            Renderer & r = renderer();
            holeX_ = MAX_X;
            holeY_ = MAX_Y;
            hole_.draw(r, 0, 0, tileRect(holeX_, holeY_));
            r.setBg(Color::RGB(128, 128, 128));
            r.fill(tileRect(holeX_, holeY_));
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

        Canvas hole_{TILE_WIDTH,TILE_HEIGHT};
        Canvas tmp_{TILE_WIDTH,TILE_HEIGHT};

        uint8_t * tileMap_ = nullptr;

        int holeX_;
        int holeY_;

        size_t shuffle_ = 0;

        Btn dir_ = Btn::Home;

        /** PNG image for the game. 
         
           from https://images6.fanpop.com/image/photos/43100000/Disney-Princesses-disney-princess-43157173-1500-1080.jpg 
         */
        static constexpr uint8_t defaultImage_[] = {
#include "SlidingPuzzle16.png.data"
        }; 

    }; 

} // namespace rckid