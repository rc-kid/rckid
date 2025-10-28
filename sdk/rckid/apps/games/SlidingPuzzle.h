#pragma once

#include "../../rckid.h"
#include "../../app.h"
#include "../../graphics/canvas.h"
#include "../../graphics/bitmap.h"
#include "../../assets/fonts/Iosevka16.h"

namespace rckid {

    /** Sliding puzzle game. 
     
        Takes images from app location on the SD card
     */
    class SlidingPuzzle : public CanvasApp<ColorRGB> {
    public:

        String name() const override { return "SlidingPuzzle"; }

        /** Games are buddgeted.
         */
        bool isBudgeted() const override { return true; }

        SlidingPuzzle(): CanvasApp<ColorRGB>{} {
            
        }
       

    protected:

        void update() override {
            if (imageSelectMode_) {

            } else {
                if (btnPressed(Btn::B)) {
                    if (tmp_ != nullptr) {
                        delete tmp_;
                        delete hole_;
                        tmp_ = nullptr;
                        hole_ = nullptr;
                    }
                    imageSelectMode_ = true;
                    btnClear(Btn::B);
                    return;
                }                    
                if (btnPressed(Btn::A)) {
                    if (tmp_ == nullptr)
                        shuffle(difficulty_);
                    else
                        return resetGame();
                }
                if ((tmp_ != nullptr) && (shuffle_ == 0) && (dir_ == Btn::Home)) {
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
                    tmp_->blit(Point::origin(), g_, tileRect(holeX_, holeY_));
                    a_.start();
                    if (dir_ != Btn::Home)
                        rumblerEffect(RumblerEffect::Nudge());
                }
            }
        }

        void draw() override {

        }


    private:

        void resetGame() {
            if (tmp_ != nullptr) {
                delete tmp_;
                delete hole_;
                tmp_ = nullptr;
                hole_ = nullptr;
            }
            /*
            if (files_.size() > 0) {
                fs::mount();
                std::string filename{STR("apps/SlidingPuzzle/" << currentItem()->filename())};
                fs::FileRead f = fs::fileRead(filename);
                g_.loadImage(PNG::fromStream(f));
                fs::unmount();
            } else {
                g_.loadImage(PNG::fromBuffer(assets::images::logo16));
            }
            imageSelectMode_ = false;
            */
            // reset any direction move & stop any outstanding animations
            dir_ = Btn::Home;
            a_.stop();
        }

        void shuffle(unsigned moves) {
            hole_ = new Bitmap{TILE_WIDTH, TILE_HEIGHT};
            tmp_ = new Bitmap{TILE_WIDTH, TILE_HEIGHT};
            // set the hole and fill in the hole canvas
            holeX_ = MAX_X;
            holeY_ = MAX_Y;
            hole_->blit(Point::origin(), g_, tileRect(holeX_, holeY_));
            g_.setBg(ColorRGB::RGB(128, 128, 128));
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
            tmp_->blit( Point::origin(), g_, t1);
            g_.blit(tilePoint(x, y), g_, t2);
            g_.blit(tilePoint(holeX_, holeY_), *tmp_);
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




        bool imageSelectMode_;

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

        Bitmap * hole_ = nullptr;
        Bitmap * tmp_ = nullptr;

        uint8_t tileMap_[NUM_TILES];

        int holeX_ = -1;
        int holeY_ = -1;
        int oldX_ = -1;
        int oldY_ = -1;
        
        uint32_t difficulty_ = 16;
        uint32_t shuffle_ = 0;

        Btn dir_ = Btn::Home;

        Timer a_{500};

    }; // rckid::SlidingPuzzle

}