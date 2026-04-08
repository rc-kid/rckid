#pragma once

#include <rckid/graphics/canvas.h>

#include <assets/OpenDyslexic24.h>
#include <assets/MetalLord64.h>

namespace rckid {

    /** The famous falling blocks game. 
     
        This time its just a port of the older versions I made. Eventually I would like to write the game usiong the script language/blocks, etc.  
     */
    class Blocks : public CanvasApp<void> {
    public:

        String name() const override { return "Blocks"; }

        Capabilities capabilities() const {
            return Capabilities {
                .canPersistState = true,
                .consumesBudget = true,
            };
        }

        static constexpr uint8_t VERSION = 1;

        Blocks(): 
            CanvasApp<void>{} 
        {
            // TODO Determine some initial state? 
            for (unsigned i = 0; i < NUM_FALLING_PIECES; ++i) {
                fallingPieces_[i].randomize();
                fallingX_[i] = random() % display::WIDTH;
                fallingY_[i] = - random() % display::HEIGHT;
            }
        }

    protected:

        // TODO for cur & next we need to save also the color
        void saveState(RandomWriteStream & into) const override {
            into.binaryWriter()
                << VERSION
                << static_cast<uint8_t>(mode_);
            if (mode_ != Mode::Game)
                return;
            into.binaryWriter()
                << score_
                << level_
                << levelCompacted_
                << countdown_
                << speed_
                << allowDown_
                << buffer(grid_, sizeof(grid_))
                << cur_
                << next_
                << x_
                << y_;
        }
        
        bool loadState(RandomReadStream & from) override {
            auto rd = from.binaryReader();
            if (read<uint8_t>(rd) != VERSION) {
                LOG(LL_WARN,  "Unsupported save version, skipping");
                return false;
            }
            mode_ = static_cast<Mode>(read<uint8_t>(rd));
            if (mode_ == Mode::Game) {
                rd
                    >> score_
                    >> level_
                    >> levelCompacted_
                    >> countdown_
                    >> speed_
                    >> allowDown_
                    >> buffer(grid_, sizeof(grid_))
                    >> cur_
                    >> next_
                    >> x_
                    >> y_;
            }
           return true;
        }

        void loop() override {
            CanvasApp<void>::loop();
            switch (mode_) {
                case Mode::Intro:
                    if (btnPressed(Btn::A) || btnPressed(Btn::Start))  {
                        resetGame();
                        mode_ = Mode::Game;
                    }
                    if (btnPressed(Btn::B)) {
                        exit();
                        btnClear(Btn::B);
                    }
                    for (unsigned i = 0; i < NUM_FALLING_PIECES; ++i) {
                        fallingY_[i] += 1;
                        if (fallingY_[i] == 240) {
                            fallingY_[i] = - 40;
                            fallingX_[i] = random() % 320;
                            fallingPieces_[i].randomize();
                        }
                        if (random() % 1000 < 10)
                            fallingPieces_[i] = fallingPieces_[i].rotate();
                    }
                    break;
                case Mode::Game: {
                    bool goDown = false;
                    if (--countdown_ == 0) {
                        goDown = true && (level_ != 1);
                        countdown_ = speed_;
                    }
                    //if (btnPressed(Btn::A))
                    //    runModal<Alert>("PAUSE", "Press A to continue...");
                    // TODO if we should, go down automatically
                    if (btnPressed(Btn::Up))
                        rotate();
                    if (btnDown(Btn::Down)) {
                        if (allowDown_) {
                            goDown = true;
                            ++score_;
                        }
                    } else {
                        allowDown_ = true;
                    }
                    // check left & right
                    if (btnPressed(Btn::Left)) {
                        if (validate(cur_, x_ - 1, y_))
                            --x_;
                        // else rumble
                    }
                    if (btnPressed(Btn::Right)) {
                        if (validate(cur_, x_ + 1, y_))
                            ++x_;
                        // else rumble
                    }
                    if (goDown) {
                        if (validate(cur_, x_, y_ + 1)) {
                            ++y_;
                            //audio_[0].setFrequency(220, 5);
                        } else {
                            addToGrid(cur_, x_, y_);
                            compactRows(y_);
                            spawn();
                            if (!validate(cur_, x_, y_))
                                gameOver();
                        }
                    }
                    if (btnPressed(Btn::B)) {
                        btnClear(Btn::B);
                        // check if we should save the high score
                        //shouldSaveHighScore();
                        mode_ = Mode::Intro;
                        //modeTimeout_ = INTRO_FRAMES_LENGTH;
                    }

                    break;
                }
            }
            
        }

        void render() override {
            canvas().fill(Color::Black());
            switch (mode_) {
                case Mode::Intro: {
                    drawIntroFallingPieces();
                    Font f{assets::MetalLord64};
                    Font f2{assets::Iosevka24};

                    int tw = f->textWidth("BLOCKS");
                    canvas().textRainbow(160 - tw / 2, 30, f, startHue_, 1024) << "BLOCKS";

                    tw = f2->textWidth("Press A to start");
                    canvas().textRainbow(160 - tw / 2, 160, f2, startHue_, -1024) << "Press A to start";
                    startHue_ += 128;
                    break;
                }
                case Mode::Game: {
                    int startX = (320 - PLAY_WIDTH * 10) / 2;
                    int startY = 238 - PLAY_HEIGHT * 10;
                    canvas().fill(Rect::XYWH(startX - 2, startY-2, 10 * PLAY_WIDTH + 3, 10 * PLAY_HEIGHT + 3), Color::DarkGray());
                    // draw the playgrid
                    for (int y = 0; y < PLAY_HEIGHT; ++y)
                        for (int x = 0; x < PLAY_WIDTH; ++x)
                            drawTile(startX + x * 10, startY + y * 10, grid(x, y));
                    // draw the current tetromino
                    drawTetromino(startX + x_ * 10, startY + y_ * 10, cur_);
                    drawTetromino(250, 100, next_);
                    Font f{assets::OpenDyslexic24};
                    canvas().text(0, 20, f, Color::Gray()) << "Score:";
                    canvas().text(0, 60, f, Color::Gray()) << "Level:";
                    String score{STR(score_)};
                    canvas().text(startX - 20 - f->textWidth(score), 40, f, Color::White()) << score;
                    String level{STR(level_)};
                    canvas().text(startX - 20 - f->textWidth(level), 80, f, Color::White()) << level;
                    break;
                }
            }
            CanvasApp<void>::render();
        }

    private:

        static constexpr uint8_t NUM_COLORS = 6;

        static constexpr Coord PLAY_WIDTH = 10;
        static constexpr Coord PLAY_HEIGHT = 22;

        enum class Mode {
            Intro, 
            Game,
        }; 

        /** Tetromino. 
         
            Can rotate, can move down, 
         */
        class Tetromino {
        public:
            enum class Kind {
                Beam,
                Square, 
                T,
                L1, 
                L2,
                Stairs1, 
                Stairs2,
            };

            /** Returns the size of tetromino grid. This is also the part of the grid that will be rotated. 
             */
            static constexpr Coord gridSize(Kind kind) {
                if (kind == Kind::Beam || kind == Kind::Square)
                    return 4;
                return 3;
            }

            Kind kind() const { return kind_; }

            uint8_t grid(int x, int y) const {
                return grid_[y][x];
            }

            /** Randomizes the tetromino's color and kind. 
             */
            void randomize() {
                kind_ = static_cast<Kind>(random() % (static_cast<uint32_t>(Kind::Stairs2) + 1));
                color_ = (random() % (NUM_COLORS - 1)) + 1;
                fillGrid();
            }

            friend void write(BinaryWriter & w, Tetromino::Kind const & kind) {
                w << static_cast<uint8_t>(kind);
            }

            friend void read(BinaryReader & r, Tetromino::Kind & kind) {
                uint8_t v;
                r >> v;
                kind = static_cast<Tetromino::Kind>(v);
            }

            friend void write(BinaryWriter & w, Tetromino const & t) {
                w << t.kind_ << t.color_;
            }

            friend void read(BinaryReader & r, Tetromino & t) {
                r >> t.kind_ >> t.color_;
                t.fillGrid();
            }

            /** Rotates the tetromino by 90 degrees. 
             
                All rotations go clockwise. 
             */
            Tetromino rotate() {
                Tetromino result{};
                result.kind_ = kind_;
                unsigned max = gridSize(kind_);
                for (unsigned x = 0; x < max; ++x)
                    for (unsigned y = 0; y < max; ++y)
                        result.grid_[y][x] = grid_[x][max - y - 1];
                return result;
            }

            void fillGrid() {
                for (unsigned x = 0; x < 4; ++x)
                    for (unsigned y = 0; y < 4; ++y)
                        grid_[y][x] = tetrominos_[static_cast<unsigned>(kind_)][y][x] ? color_ : 0;
            }

        private:
            Kind kind_;
            uint8_t color_;
            uint8_t grid_[4][4];
        }; 

        void resetGame() {
            level_ = 1;
            score_ = 0;
            speed_ = getLevelSpeed(1);
            for (unsigned i = 0; i < PLAY_WIDTH * PLAY_HEIGHT; ++i)
                grid_[i] = 0;
            next_.randomize();
            spawn();
            countdown_ = speed_;
            mode_ = Mode::Intro;
            //modeTimeout_ = INTRO_FRAMES_LENGTH;
        }

        /** Game over.

            Display game over dialg and then reset the game. 

            TODO store high score, etc.          
         */
        void gameOver() {
            //runModal<Alert>("GAME OVER", "Press A to continue...", ColorRGB{32, 0, 0});
            //shouldSaveHighScore();
            resetGame();
        }

        uint8_t grid(int x, int y) {
            // return the unattainable 0xff color which will stop rotations or falls when out of bounds
            if (x < 0 || y < 0 || x >= PLAY_WIDTH || y >= PLAY_HEIGHT)
                return 0xff; 
            return grid_[y * PLAY_WIDTH + x];
        }

        void setGrid(int x, int y, uint8_t color) {
            ASSERT(x >= 0 && y >= 0 && x < PLAY_WIDTH && y < PLAY_HEIGHT);
            grid_[y * PLAY_WIDTH + x] = color;
        }

        /** Validates the tetromino's position wrt the board.
         */
        bool validate(Tetromino const & t, int tx, int ty) {
            for (int x = 0; x < 4; ++x)
                for (int y = 0; y < 4; ++y)
                    if (t.grid(x,y) != 0 && grid(tx + x, ty + y) != 0)
                        return false;
            return true;
        }

        /** Draws single tile at given location (absolute coordinates) 
         */
        void drawTile(int x, int y, uint8_t color) {
            if (color <= NUM_COLORS) {
                canvas().fill(Rect::XYWH(x, y, 9, 9), palette_[color]);
            } else {
                // TODO effects, etc
            }
        }

        void drawTetromino(int x, int y, Tetromino const & t) {
            int max = Tetromino::gridSize(t.kind());
            for (int ty = 0; ty < max; ++ty)
                for (int tx = 0; tx < max; ++tx) {
                    uint8_t c = t.grid(tx, ty);
                    if (c != 0)
                        drawTile(x + tx * 10, y + ty * 10, c);
                }
        }

        /** Spawns next tetromino. 
         */
        void spawn() {
            cur_ = next_;
            next_.randomize();
            x_ = 3;
            if (cur_.kind() == Tetromino::Kind::Beam || cur_.kind() == Tetromino::Kind::Square)
                y_ = -1;
            else 
                y_ = 0;
            // don't allow the next tile to go immediately down
            allowDown_ = false;
        }

        /** Rotates the current tetromino, if possible.
         */
        void rotate() {
            Tetromino tr = cur_.rotate();
            if (validate(tr, x_, y_))
                cur_ = tr;
            // TODO else rumble bad
            //else
            //   rumble()
            //audio_[1].setFrequency(440, 30);
           //rumblerEffect(RumblerEffect::Nudge());
        }

        void addToGrid(Tetromino const & t, int x, int y) {
            for (int ty = 0; ty < 4; ++ty)
                for (int tx = 0; tx < 4; ++tx) {
                    uint8_t c = t.grid(tx, ty);
                    if (c != 0)
                        setGrid(x + tx, y + ty, c);
                }
        }

        /** Checks row compacting when the tile lands. 
         */
        void compactRows(int startRow) {
            uint32_t compacted = 0;
            for (int y = startRow; y < startRow + 4; ++y) {
                bool compact = true;
                for (int x = 0; x < PLAY_WIDTH; ++x) {
                    uint8_t gc = grid(x, y);
                    if (gc == 0 || gc == 0xff) {
                        compact = false;
                        break;
                    }
                }
                if (compact) {
                    ++compacted;
                    for (int yy = y; yy > 1; --yy)
                        for (int x = 0; x < PLAY_WIDTH; ++x)
                            setGrid(x, yy, grid(x, yy - 1));
                    for (int x = 0; x < PLAY_WIDTH; ++x)
                        setGrid(x, 0, 0);
                }
            }
            switch (compacted) {
                case 1:
                    score_ += 100 * level_;
                    //audio_[3].setFrequency(220, 250);
                    break;
                case 2:
                    score_ += 300 * level_;
                    //audio_[3].setFrequency(330, 500);
                    break;
                case 3:
                    score_ += 500 * level_;
                    //audio_[3].setFrequency(440, 750);
                    break;
                case 4:
                    score_ += 800 * level_;
                    //audio_[3].setFrequency(660, 1000);
                    break;
                default:
                    // no points awarded
                    break;
            }
            levelCompacted_ += compacted;
            if (levelCompacted_ > 10) {
                ++level_;
                speed_ = getLevelSpeed(level_);
                levelCompacted_ -= 10;
                // TODO
                //rumblerEffect(RumblerEffect::OK());
            } else if (compacted) {
                // TODO
                //rumblerEffect(RumblerEffect::Nudge());
            }
        }

        void drawIntroFallingPieces() {
            for (unsigned i = 0; i < NUM_FALLING_PIECES; ++i)
                drawTetromino(fallingX_[i], fallingY_[i], fallingPieces_[i]);
        }

        Mode mode_ = Mode::Intro;

        /** The play area.  
         
            Stored value indicates color, 0 for black, which means empty. 
         */
        uint8_t grid_[PLAY_WIDTH * PLAY_HEIGHT]; 

        Tetromino cur_;
        Coord x_;
        Coord y_;
        Tetromino next_;

        uint32_t level_ = 1;
        uint32_t levelCompacted_ = 0;
        uint32_t score_ = 0;
        uint32_t speed_;
        uint32_t countdown_;
        // used to disable immediate go down of spawned tiles after previoius one has been landed
        bool allowDown_ = true;






        // falling tetrominos in the intro
        static constexpr unsigned NUM_FALLING_PIECES = 10;
        Tetromino fallingPieces_[NUM_FALLING_PIECES];
        int fallingX_[NUM_FALLING_PIECES];
        int fallingY_[NUM_FALLING_PIECES];
        // hue for the tetris logo
        uint16_t startHue_ = 0;





        /** Default grids of all 7 types. 
         */
        static constexpr uint8_t tetrominos_[][4][4] {
            {
                {0,0,0,0},
                {1,1,1,1},
                {0,0,0,0},
                {0,0,0,0},
            },
            {
                {0,0,0,0},
                {0,1,1,0},
                {0,1,1,0},
                {0,0,0,0},
            },
            {
                {0,1,0,0},
                {1,1,1,0},
                {0,0,0,0},
                {0,0,0,0},
            },
            {
                {1,0,0,0},
                {1,1,1,0},
                {0,0,0,0},
                {0,0,0,0},
            },
            {
                {0,0,1,0},
                {1,1,1,0},
                {0,0,0,0},
                {0,0,0,0},
            },
            {
                {1,1,0,0},
                {0,1,1,0},
                {0,0,0,0},
                {0,0,0,0},
            },
            {
                {0,1,1,0},
                {1,1,0,0},
                {0,0,0,0},
                {0,0,0,0},
            },
        };


        static constexpr Color::RGB565 palette_[] = {
            Color::Black(),
            Color::Red(), 
            Color::Green(),
            Color::Yellow(),
            Color::Blue(),
            Color::Magenta(), 
            Color::Cyan(),
        };

        static uint32_t getLevelSpeed(uint32_t level) {
            switch (level) {
                case 1: return 60;
                case 2: return 53;
                case 3: return 49;
                case 4: return 45;
                case 5: return 41;
                case 6: return 37;
                case 7: return 33;
                case 8: return 28;
                case 9: return 22;
                case 10: return 17;
                case 11: return 11;
                case 12: return 10;
                case 13: return 9;
                case 14: return 8;
                case 15: return 7;
                case 16: return 6;
                case 17: return 6;
                case 18: return 5;
                case 19: return 5;
                case 20: return 4;
                case 21: return 4;
                default:
                    return 3;
            }
        }


    }; // rckid::Blocks

} // namespace rckid