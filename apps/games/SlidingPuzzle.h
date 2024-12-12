#pragma once

#include <rckid/app.h>
#include <rckid/graphics/color.h>
#include <rckid/graphics/png.h>
#include <rckid/graphics/canvas.h>
#include <rckid/ui/timer.h>
#include <rckid/utils/interpolation.h>
#include <rckid/ui/carousel.h>
#include <rckid/assets/fonts/OpenDyslexic48.h>
#include <rckid/assets/fonts/OpenDyslexic32.h>
#include <rckid/assets/icons64.h>
#include <rckid/filesystem.h>
#include <rckid/assets/images.h>


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

        class Item : public MenuItem {
        public:

            Item(filesystem::Entry const & e) {
                text_ = e.name();
            }

            std::string const & filename() const { return text_; }    

            void text(std::string & text) const override {
                text = filesystem::stem(text_); 
            }

            bool icon(Bitmap<ColorRGB> &bmp) const override {
                bmp.loadImage(PNG::fromBuffer(assets::icons64::screenshot));
                return true;
            }

        private:

            std::string text_;
        };


        SlidingPuzzle(): GraphicsApp{ARENA(Canvas<Color>{320, 240})} {
            filesystem::mount();
            imageSelectMode_ = loadFolder();
            if (! imageSelectMode_) {
                resetGame();
            }
            g_.setFont(assets::font::OpenDyslexic32::font);
        }

        ~SlidingPuzzle() {
            filesystem::unmount();
        }

        void reset() {
            shuffle_ = 0;
            holeX_ = -1;
            holeY_ = -1;
        }

        void onFocus() override {
            App::onFocus();
            //PNG png = PNG::fromBuffer(defaultImage_, sizeof(defaultImage_));
            //g_.loadImage(PNG::fromBuffer(defaultImage_, sizeof(defaultImage_)));
            /*
            png.decode([&](ColorRGB * line, int lineNum, int lineWidth){
                Renderer & r = renderer();
                for (int i = 0; i < lineWidth; ++i) {
                    ColorRGB c = line[i];
                    r.setPixelAt(i, lineNum, Color::RGB(c.r(), c.g(), c.b()));
                }
            });
            */
            //shuffle_ = 0;
            //holeX_ = -1;
            //holeY_ = -1;
        }

        void update() override {
            if (imageSelectMode_) {
                if (btnPressed(Btn::Left)) {
                    i_ = (i_ == 0) ? files_.size() - 1 : i_ - 1;
                    carousel_.moveLeft(files_[i_]);    
                    rumbleNudge();

                }
                if (btnPressed(Btn::Right)) {
                    ++i_;
                    if (i_ >= files_.size())
                        i_ = 0;
                    carousel_.moveRight(files_[i_]);    
                    rumbleNudge();
                }
                if (btnPressed(Btn::Up) || btnPressed(Btn::A)) {
                    resetGame();
                }
                if (btnPressed(Btn::Down) || btnPressed(Btn::B)) {
                    exit();
                    btnPressedClear(Btn::B);
                }
                if (btnPressed(Btn::Select)) {
                    if (difficulty_ < 64)
                        difficulty_ *= 2;
                    else 
                        difficulty_ = 4;
                }
            } else {
                if (btnPressed(Btn::B)) {
                    if (tmp_ != nullptr) {
                        delete tmp_;
                        delete hole_;
                        tmp_ = nullptr;
                        hole_ = nullptr;
                    }
                    if (files_.size() == 0) {
                        exit();
                    } else {
                        imageSelectMode_ = true;
                        btnPressedClear(Btn::B);
                    }
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
                        rumbleNudge();
                }
            }
        }

        void draw() override {
            if (imageSelectMode_) {
                g_.setBg(Color{0, 0, 0});
                g_.fill();
                carousel_.drawOn(g_, Rect::XYWH(0, 160, 320, 80));
                char const * d = "???"; 
                switch (difficulty_) {
                    case 4:
                        d = "toddler";
                        break;
                    case 8:
                        d = "baby";
                        break;
                    case 16:
                        d = "child";
                        break;
                    case 32:
                        d = "grownup";
                        break;
                    case 64:
                        d = "wise old elf";
                        break;
                }
                int w = g_.font().textWidth(d);
                g_.text(160 - w / 2, 80) << d;
            } else {
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
                        g_.blit(tilePoint(holeX_, holeY_) - Point{interpolation::easingCos(a_, 0, TILE_WIDTH), 0}, *tmp_);
                        break;
                    case Btn::Right:
                        g_.fill(tileRect(holeX_, holeY_));
                        g_.blit(tilePoint(holeX_, holeY_) + Point{interpolation::easingCos(a_, 0, TILE_WIDTH), 0}, *tmp_);
                        break;
                    case Btn::Up:
                        g_.fill(tileRect(holeX_, holeY_));
                        g_.blit(tilePoint(holeX_, holeY_) - Point{0, interpolation::easingCos(a_, 0, TILE_HEIGHT)}, *tmp_);
                        break;
                    case Btn::Down: 
                        g_.fill(tileRect(holeX_, holeY_));
                        g_.blit(tilePoint(holeX_, holeY_) + Point{0, interpolation::easingCos(a_, 0, TILE_HEIGHT)}, *tmp_);
                        break;
                    default:
                        break; // nothing to do for other controls
                }
                if (dir_ != Btn::Home && !a_.running() && shuffle_ == 0) {
                    dir_ = Btn::Home;
                    if (swapTileMap(oldX_, oldY_, holeX_, holeY_)) {
                        // tada, game is finished
                        g_.blit(tilePoint(holeX_, holeY_), *hole_);
                        holeX_ = -1;
                        holeY_ = -1;
                    }
                }
            }
        }

        void resetGame() {
            if (tmp_ != nullptr) {
                delete tmp_;
                delete hole_;
                tmp_ = nullptr;
                hole_ = nullptr;
            }
            if (files_.size() > 0) {
                filesystem::mount();
                std::string filename{STR("apps/SlidingPuzzle/" << currentItem()->filename())};
                filesystem::FileRead f = filesystem::fileRead(filename);
                g_.loadImage(PNG::fromStream(f));
                filesystem::unmount();
            } else {
                g_.loadImage(PNG::fromBuffer(assets::images::logo16));
            }
            imageSelectMode_ = false;
            // reset any direction move & stop any outstanding animations
            dir_ = Btn::Home;
            a_.stop();
        }

        void shuffle(unsigned moves) {
            hole_ = new Bitmap<ColorRGB>{TILE_WIDTH, TILE_HEIGHT};
            tmp_ = new Bitmap<ColorRGB>{TILE_WIDTH, TILE_HEIGHT};
            // set the hole and fill in the hole canvas
            holeX_ = MAX_X;
            holeY_ = MAX_Y;
            hole_->blit(Point::origin(), g_, tileRect(holeX_, holeY_));
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

        bool loadFolder() {
            filesystem::Folder f = filesystem::folderRead("apps/SlidingPuzzle");
            if (!f.good())
                return false;
            files_.clear();
            for (auto const & i : f) {
                LOG("Loadded item " << i.name());
                files_.add(new Item{i});
            }
            i_ = 0;
            carousel_.moveDown(files_[i_]);
            return true;
        }

        Item * currentItem() { return (Item*) & files_[i_]; }



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

        Bitmap<Color> * hole_ = nullptr;
        Bitmap<Color> * tmp_ = nullptr;

        uint8_t tileMap_[NUM_TILES];

        int holeX_ = -1;
        int holeY_ = -1;
        int oldX_ = -1;
        int oldY_ = -1;
        
        uint32_t difficulty_ = 16;
        uint32_t shuffle_ = 0;

        Btn dir_ = Btn::Home;

        Timer a_{500};


        Menu files_;
        uint32_t i_ = 0;
        bool imageSelectMode_ = false;

        Carousel carousel_{assets::font::OpenDyslexic48::font};
    }; 

} // namespace rckid