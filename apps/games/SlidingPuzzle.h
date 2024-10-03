#pragma once

#include <rckid/app.h>
#include <rckid/graphics/color.h>
#include <rckid/graphics/png.h>
#include <rckid/graphics/canvas.h>
#include <rckid/graphics/animation.h>

namespace rckid {

    /** A simple sliding puzzle game, silimar to FifteenPuzzle. 
     
        More of a demonstration of what can be done. 

        TODO:

          - support different picture sizes
          - support multiple tile sizes
          - dpad wiring seems counterintuitive atm
          - add accelerometer as input

     */
    class SlidingPuzzle : public GraphicsApp<Canvas<ColorRGB>> {
    public:

        static void run() {
            SlidingPuzzle p;
            p.loop();
        }

    protected:

        SlidingPuzzle(): GraphicsApp{Canvas<Color>{320, 240}} {}

        void onFocus() override {
            App::onFocus();
            //PNG png = PNG::fromBuffer(defaultImage_, sizeof(defaultImage_));
            g_.loadImage(PNG::fromBuffer(defaultImage_, sizeof(defaultImage_)));
            /*
            png.decode([&](ColorRGB * line, int lineNum, int lineWidth){
                Renderer & r = renderer();
                for (int i = 0; i < lineWidth; ++i) {
                    ColorRGB c = line[i];
                    r.setPixelAt(i, lineNum, Color::RGB(c.r(), c.g(), c.b()));
                }
            });
            */
            shuffle_ = 0;
            holeX_ = -1;
            holeY_ = -1;
        }

        void update() override {
            if (btnPressed(Btn::B))
                exit();
            if (btnPressed(Btn::Start))
                resetGame(20);
                // shuffle_ = 20;
            if (shuffle_ == 0 && dir_ == Btn::Home) {
                oldX_ = holeX_;
                oldY_ = holeY_;
                if (btnPressed(Btn::Left) && canMoveLeft()) {
                    holeX_ += 1;
                    dir_ = Btn::Left;
                } else if (btnPressed(Btn::Right) && canMoveRight()) {
                    holeX_ -= 1;
                    dir_ = Btn::Right;
                } else if (btnPressed(Btn::Up) && canMoveUp()) {
                    holeY_ += 1;
                    dir_ = Btn::Up;
                } else if (btnPressed(Btn::Down) && canMoveDown()) {
                    holeY_ -= 1;
                    dir_ = Btn::Down;
                } else {
                    return;
                }
                tmp_.blit(Point::origin(), g_, tileRect(holeX_, holeY_));
                a_.start();
                //if (dir_ != Btn::Home)
                //    setRumbler(RumblerEffect::Nudge());
            }
        }

        void draw() override {
            a_.update();
            if (shuffle_ > 0) {
                shuffleMove();
                shuffleMove();
                shuffleMove();
                --shuffle_;
            } 
            switch (dir_) {
                case Btn::Left:
                    g_.fill(tileRect(holeX_, holeY_));
                    g_.blit(tilePoint(holeX_, holeY_) - Point{a_.interpolate(0, TILE_WIDTH), 0}, tmp_);
                    break;
                case Btn::Right:
                    g_.fill(tileRect(holeX_, holeY_));
                    g_.blit(tilePoint(holeX_, holeY_) + Point{a_.interpolate(0, TILE_WIDTH), 0}, tmp_);
                    break;
                case Btn::Up:
                    g_.fill(tileRect(holeX_, holeY_));
                    g_.blit(tilePoint(holeX_, holeY_) - Point{0, a_.interpolate(0, TILE_HEIGHT)}, tmp_);
                    break;
                case Btn::Down: 
                    g_.fill(tileRect(holeX_, holeY_));
                    g_.blit(tilePoint(holeX_, holeY_) + Point{0, a_.interpolate(0, TILE_HEIGHT)}, tmp_);
                    break;
                default:
                    break; // nothing to do for other controls
            }
            if (dir_ != Btn::Home && !a_.running() && shuffle_ == 0) {
                dir_ = Btn::Home;
                if (swapTileMap(oldX_, oldY_, holeX_, holeY_)) {
                    // tada, game is finished
                    g_.blit(tilePoint(holeX_, holeY_), hole_);
                    holeX_ = -1;
                    holeY_ = -1;
                }
            }
        }

        void resetGame(unsigned moves) {
            g_.loadImage(PNG::fromBuffer(defaultImage_, sizeof(defaultImage_)));
            // set the hole and fill in the hole canvas
            holeX_ = MAX_X;
            holeY_ = MAX_Y;
            hole_.blit(Point::origin(), g_, tileRect(holeX_, holeY_));
            g_.setBg(Color{128, 128, 128});
            g_.fill(tileRect(holeX_, holeY_));
            // reset the tilemap
            for (int i = 0; i < NUM_TILES; ++i)
                tileMap_[i] = i;
            // set the number of shuffle moves
            shuffle_ = moves;
            // reset any direction move & stop any outstanding animations
            dir_ = Btn::Home;
            a_.stop();
        }

        void shuffleMove() {
            // figure out which move to make
            int x = holeX_;
            int y = holeY_;
            int t = random() % 4;
            while (true) {
                if (t == 0 && canMoveLeft())
                    holeX_ += 1;
                else if (t == 1 && canMoveRight())
                    holeX_ -= 1;
                else if (t == 2 && canMoveUp())
                    holeY_ += 1;
                else if (t == 3 && canMoveDown())
                    holeY_ -= 1;
                else {
                    t = (t + 1) % 4;
                    continue;
                }
                break;
            }
            // swap the tiles
            Rect t1 = tileRect(x, y);
            Rect t2 = tileRect(holeX_, holeY_);
            tmp_.blit( Point::origin(), g_, t1);
            g_.blit(tilePoint(x, y), g_, t2);
            g_.blit(tilePoint(holeX_, holeY_), tmp_);
            // update th7e tilemap
            swapTileMap(x, y, holeX_, holeY_);
        }

        bool swapTileMap(int x1, int y1, int x2, int y2) {
            std::swap(tileMap_[y1 * MAX_X + x1], tileMap_[y2 * MAX_X + x2]);
            for (int i = 0; i < NUM_TILES; ++i)
                if (tileMap_[i] != i)
                    return false;
            return true;
        }

        bool canMoveLeft() { return holeX_ < MAX_X && holeX_ >= 0; }
        bool canMoveRight() { return holeX_ > 0; }
        bool canMoveUp() { return holeY_ < MAX_Y && holeY_ >= 0; }
        bool canMoveDown() { return holeY_ > 0; }

    private:

        static constexpr int TILE_WIDTH = 80;
        static constexpr int TILE_HEIGHT = 80;

        static constexpr int MAX_X = 320 / TILE_WIDTH - 1;
        static constexpr int MAX_Y = 240 / TILE_HEIGHT - 1;

        static constexpr int NUM_TILES = (MAX_X + 1) * (MAX_Y + 1);

        Rect tileRect(int x, int y) {
            return Rect::XYWH(x * TILE_WIDTH, y * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT);
        }

        Point tilePoint(int x, int y) {
            return Point{ x * TILE_WIDTH, y * TILE_HEIGHT};
        }

        Bitmap<Color> hole_{TILE_WIDTH,TILE_HEIGHT};
        Bitmap<Color> tmp_{TILE_WIDTH,TILE_HEIGHT};

        uint8_t tileMap_[NUM_TILES];

        int holeX_ = -1;
        int holeY_ = -1;
        int oldX_ = -1;
        int oldY_ = -1;

        size_t shuffle_ = 0;

        Btn dir_ = Btn::Home;

        Animation a_{500};

        /** PNG image for the game. 
         
           from https://images6.fanpop.com/image/photos/43100000/Disney-Princesses-disney-princess-43157173-1500-1080.jpg 
         */
        static constexpr uint8_t defaultImage_[] = {
#include "SlidingPuzzle16.png.inc"
        }; 

    }; 

} // namespace rckid