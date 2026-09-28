#pragma once

#include <vector>

#include <rckid/graphics/canvas.h>
#include <rckid/ui/animation.h>
#include <rckid/ui/carousel.h>

#include <assets/OpenDyslexic32.h>
#include <assets/icons_64.h>
#include <assets/images.h>

namespace rckid {

    /** A simple sliding puzzle game, similar to FifteenPuzzle. 
     
        More of a demonstration of what can be done. The puzzle is played directly in the app's framebuffer - the picture is loaded into the canvas and the tiles are then moved around by blitting parts of the canvas onto itself, which is why the game keeps no board state beyond the tile map used to tell whether the puzzle has been solved.

        TODO:

          - support different picture sizes
          - support multiple tile sizes
          - dpad wiring seems counterintuitive atm
          - add accelerometer as input
     */
    class SlidingPuzzle : public CanvasApp<void> {
    public:

        String name() const override { return "SlidingPuzzle"; }

        Capabilities capabilities() const override {
            return Capabilities {
                .consumesBudget = true,
            };
        }

        SlidingPuzzle(): CanvasApp<void>{} {
            // the carousel is a plain UI widget rendered into our canvas, so it has no parent to size it or to apply the style for it
            carousel_.setRect(Rect::WH(CAROUSEL_RECT.width(), CAROUSEL_RECT.height()));
            carousel_.applyStyle();
            imageSelectMode_ = loadFolder();
            if (imageSelectMode_)
                showImage(Direction::Down);
            else
                resetGame();
        }

    protected:

        void loop() override {
            CanvasApp<void>::loop();
            if (imageSelectMode_) {
                if (btnPressed(Btn::Left)) {
                    i_ = (i_ == 0) ? files_.size() - 1 : i_ - 1;
                    showImage(Direction::Left);
                    rumbler::nudge();
                }
                if (btnPressed(Btn::Right)) {
                    ++i_;
                    if (i_ >= files_.size())
                        i_ = 0;
                    showImage(Direction::Right);
                    rumbler::nudge();
                }
                if (btnPressed(Btn::Up) || btnPressed(Btn::A))
                    resetGame();
                if (btnPressed(Btn::Down) || btnPressed(Btn::B)) {
                    btnClear(Btn::B);
                    exit();
                }
                if (btnPressed(Btn::Select)) {
                    if (difficulty_ < 64)
                        difficulty_ *= 2;
                    else 
                        difficulty_ = 4;
                }
            } else {
                if (btnPressed(Btn::B)) {
                    releaseTiles();
                    if (files_.empty()) {
                        btnClear(Btn::B);
                        exit();
                    } else {
                        imageSelectMode_ = true;
                        showImage();
                        btnClear(Btn::B);
                    }
                    return;
                }                    
                if (btnPressed(Btn::A)) {
                    if (! started_)
                        shuffle(difficulty_);
                    else
                        return resetGame();
                }
                if (started_ && (shuffle_ == 0) && (dir_ == Btn::Home)) {
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
                    tmp_->blit(Point{0, 0}, canvas(), tileRect(holeX_, holeY_));
                    startAnimation();
                    rumbler::nudge();
                }
            }
        }

        void render() override {
            // the image selection carousel is a UI widget rendered into our canvas, which means we are responsible for the per frame UI bookkeeping (animations) ourselves
            ui::Widget::renderEssentials(rect());
            if (imageSelectMode_) {
                canvas().fill(Color::Black());
                carousel_.renderOn(canvas(), CAROUSEL_RECT.topLeft());
                char const * d = difficultyName();
                Font f{assets::OpenDyslexic32};
                Coord w = f->textWidth(d);
                canvas().text(160 - w / 2, 80, f, Color::White()) << d;
            } else {
                // note that the canvas is *not* cleared here - the puzzle itself lives in it and only the moving tile is repainted every frame
                if (shuffle_ > 0) {
                    shuffleMove();
                    shuffleMove();
                    shuffleMove();
                    --shuffle_;
                } 
                bool done = animationDone();
                switch (dir_) {
                    case Btn::Left:
                        canvas().fill(tileRect(holeX_, holeY_), HOLE_COLOR);
                        blitTmp(tilePoint(holeX_, holeY_) - Point{animationOffset(TILE_WIDTH), 0});
                        break;
                    case Btn::Right:
                        canvas().fill(tileRect(holeX_, holeY_), HOLE_COLOR);
                        blitTmp(tilePoint(holeX_, holeY_) + Point{animationOffset(TILE_WIDTH), 0});
                        break;
                    case Btn::Up:
                        canvas().fill(tileRect(holeX_, holeY_), HOLE_COLOR);
                        blitTmp(tilePoint(holeX_, holeY_) - Point{0, animationOffset(TILE_HEIGHT)});
                        break;
                    case Btn::Down: 
                        canvas().fill(tileRect(holeX_, holeY_), HOLE_COLOR);
                        blitTmp(tilePoint(holeX_, holeY_) + Point{0, animationOffset(TILE_HEIGHT)});
                        break;
                    default:
                        break; // nothing to do for other controls
                }
                if (dir_ != Btn::Home && done && shuffle_ == 0) {
                    dir_ = Btn::Home;
                    if (swapTileMap(oldX_, oldY_, holeX_, holeY_)) {
                        // tada, game is finished
                        canvas().blit(tilePoint(holeX_, holeY_), *hole_, Rect::WH(TILE_WIDTH, TILE_HEIGHT));
                        holeX_ = -1;
                        holeY_ = -1;
                    }
                }
            }
            CanvasApp<void>::render();
        }

        void resetGame() {
            releaseTiles();
            canvas().fill(Color::Black());
            Bitmap img = files_.empty() 
                ? Bitmap{ImageSource{assets::images::logo}}
                : Bitmap{ImageSource{fs::join(PUZZLE_FOLDER, files_[i_]), fs::Drive::SD}};
            if (! img.empty())
                canvas().blit(Point{(320 - img.width()) / 2, (240 - img.height()) / 2}, img);
            imageSelectMode_ = false;
            // reset any direction move & stop any outstanding animations
            dir_ = Btn::Home;
            animating_ = false;
        }

        void shuffle(uint32_t moves) {
            hole_ = std::make_unique<Canvas>(TILE_WIDTH, TILE_HEIGHT);
            tmp_ = std::make_unique<Canvas>(TILE_WIDTH, TILE_HEIGHT);
            started_ = true;
            // set the hole and fill in the hole canvas
            holeX_ = MAX_X;
            holeY_ = MAX_Y;
            hole_->blit(Point{0, 0}, canvas(), tileRect(holeX_, holeY_));
            canvas().fill(tileRect(holeX_, holeY_), HOLE_COLOR);
            // reset the tilemap
            for (int i = 0; i < NUM_TILES; ++i)
                tileMap_[i] = static_cast<uint8_t>(i);
            // set the number of shuffle moves
            shuffle_ = moves;
            // reset any direction move & stop any outstanding animations
            dir_ = Btn::Home;
            animating_ = false;
        }

        void shuffleMove() {
            // figure out which move to make
            int x = holeX_;
            int y = holeY_;
            int t = static_cast<int>(random() % 4);
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
            tmp_->blit(Point{0, 0}, canvas(), t1);
            canvas().blit(tilePoint(x, y), canvas(), t2);
            canvas().blit(tilePoint(holeX_, holeY_), *tmp_, Rect::WH(TILE_WIDTH, TILE_HEIGHT));
            // update the tilemap
            swapTileMap(x, y, holeX_, holeY_);
        }

        bool swapTileMap(int x1, int y1, int x2, int y2) {
            // NOTE the mkII version indexed the tile map by y * MAX_X + x, which is off by one row stride (MAX_X is the largest valid column, not the number of columns) and made the solved check unreliable. Fixed here.
            std::swap(tileMap_[y1 * COLS + x1], tileMap_[y2 * COLS + x2]);
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
            files_.clear();
            fs::readFolder(PUZZLE_FOLDER, fs::Drive::SD, [this](fs::FolderEntry const & e) {
                if (e.isFolder)
                    return;
                LOG(LL_INFO, "Loaded item " << e.name);
                files_.push_back(e.name);
            });
            i_ = 0;
            return ! files_.empty();
        }

    private:

        static constexpr char const * PUZZLE_FOLDER = "/files/images/sliding-puzzle";

        static constexpr Coord TILE_WIDTH = 80;
        static constexpr Coord TILE_HEIGHT = 80;

        static constexpr int COLS = 320 / TILE_WIDTH;
        static constexpr int ROWS = 240 / TILE_HEIGHT;

        static constexpr int MAX_X = COLS - 1;
        static constexpr int MAX_Y = ROWS - 1;

        static constexpr int NUM_TILES = COLS * ROWS;

        static inline Color const HOLE_COLOR = Color::RGB(128, 128, 128);

        /** Where the image selection carousel sits on the screen. 
         */
        static constexpr Rect CAROUSEL_RECT = Rect::XYWH(0, 160, 320, 80);

        /** Shows the currently selected image in the carousel, optionally animating it in from the given direction.
         */
        void showImage(std::optional<Direction> dir = std::nullopt) {
            String name = fs::stem(files_[i_]);
            Bitmap icon{ImageSource{assets::icons_64::picture}};
            if (dir.has_value())
                carousel_.set(std::move(name), std::move(icon), dir.value());
            else
                carousel_.set(std::move(name), std::move(icon));
        }

        char const * difficultyName() const {
            switch (difficulty_) {
                case 4:
                    return "toddler";
                case 8:
                    return "baby";
                case 16:
                    return "child";
                case 32:
                    return "grownup";
                case 64:
                    return "wise old elf";
                default:
                    return "???";
            }
        }

        void releaseTiles() {
            hole_.reset();
            tmp_.reset();
            started_ = false;
        }

        void blitTmp(Point where) {
            canvas().blit(where, *tmp_, Rect::WH(TILE_WIDTH, TILE_HEIGHT));
        }

        /** \name Tile slide animation
         
            The mkII SDK used a Timer and a cosine easing function. The new SDK has the easing functions in the ui namespace, the timing is kept here as the game owns the animation itself.
         */
        //@{
        void startAnimation() {
            animating_ = true;
            animationStartUs_ = time::uptimeUs();
        }

        uint32_t animationElapsedMs() const {
            return static_cast<uint32_t>((time::uptimeUs() - animationStartUs_) / 1000);
        }

        bool animationDone() const {
            return ! animating_ || animationElapsedMs() >= ANIMATION_DURATION_MS;
        }

        Coord animationOffset(Coord distance) const {
            uint32_t elapsed = animationElapsedMs();
            if (! animating_ || elapsed >= ANIMATION_DURATION_MS)
                return distance;
            return ui::easing::inOut(FixedRatio{elapsed, ANIMATION_DURATION_MS}).scale(distance);
        }
        //@}

        static constexpr uint32_t ANIMATION_DURATION_MS = 500;

        Rect tileRect(int x, int y) {
            return Rect::XYWH(x * TILE_WIDTH, y * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT);
        }

        Point tilePoint(int x, int y) {
            return Point{ x * TILE_WIDTH, y * TILE_HEIGHT};
        }

        // the contents of the hole tile, put back when the puzzle is solved, and a scratch tile used when moving tiles around
        unique_ptr<Canvas> hole_;
        unique_ptr<Canvas> tmp_;
        bool started_ = false;

        uint8_t tileMap_[NUM_TILES];

        int holeX_ = -1;
        int holeY_ = -1;
        int oldX_ = -1;
        int oldY_ = -1;
        
        uint32_t difficulty_ = 16;
        uint32_t shuffle_ = 0;

        // Btn::Home means no tile is moving
        Btn dir_ = Btn::Home;
        bool animating_ = false;
        uint64_t animationStartUs_ = 0;

        std::vector<String> files_;
        uint32_t i_ = 0;
        bool imageSelectMode_ = false;

        ui::Carousel carousel_;

    }; // rckid::SlidingPuzzle

} // namespace rckid
