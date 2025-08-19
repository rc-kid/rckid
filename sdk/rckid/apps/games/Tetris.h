#include "../../rckid.h"
#include "../../app.h"
#include "../../graphics/canvas.h"
#include "../../assets/fonts/Iosevka24.h"
#include "../../assets/fonts/MetalLord64.h"

namespace rckid {

    /** A simple tetris game. 
     */
    class Tetris : public CanvasApp<ColorRGB> {
    public:

        Tetris(): CanvasApp<ColorRGB>{} {
            for (unsigned i = 0; i < NUM_FALLING_PIECES; ++i) {
                fallingPieces_[i].randomize();
                fallingX_[i] = random() % 320;
                fallingY_[i] = - random() % 240;
            }
        }
       

    protected:

        void update() override {
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
            CanvasApp<ColorRGB>::update();
        }

        void draw() override {
            g_.fill();

            drawIntroFallingPieces();
            Font const & f = Font::fromROM<assets::MetalLord64>();
            Font const & f2 = Font::fromROM<assets::Iosevka24>();

            int tw = f.textWidth("TETRIS");
            g_.textRainbow(160 - tw / 2, 30, f, startHue_, 1024) << "TETRIS";

            tw = f2.textWidth("Press A to start");
            g_.textRainbow(160 - tw / 2, 160, f2, startHue_, -1024) << "Press A to start";
            startHue_ += 128;

            CanvasApp<ColorRGB>::draw();
        }
    
    private:

        static constexpr uint8_t NUM_COLORS = 6;

        static constexpr Coord PLAY_WIDTH = 10;
        static constexpr Coord PLAY_HEIGHT = 22;

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
                uint8_t color = (random() % (NUM_COLORS - 1)) + 1;
                for (unsigned x = 0; x < 4; ++x)
                    for (unsigned y = 0; y < 4; ++y)
                        grid_[y][x] = tetrominos_[static_cast<unsigned>(kind_)][y][x] ? color : 0;
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

        private:
            Kind kind_;
            uint8_t grid_[4][4];
        }; 


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
                g_.fill(palette_[color], Rect::XYWH(x, y, 9, 9));
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




        void drawIntroFallingPieces() {
            for (unsigned i = 0; i < NUM_FALLING_PIECES; ++i)
                drawTetromino(fallingX_[i], fallingY_[i], fallingPieces_[i]);
        }

        /** The play area.  
         
            Stored value indicates color, 0 for black, which means empty. 
         */
        uint8_t grid_[PLAY_WIDTH * PLAY_HEIGHT]; 


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


        static constexpr ColorRGB palette_[] = {
            ColorRGB::Black(),
            ColorRGB::Red(), 
            ColorRGB::Green(),
            ColorRGB::Yellow(),
            ColorRGB::Blue(),
            ColorRGB::Violet(), 
            ColorRGB::Cyan(),
        };


    }; // rckid::Tetris

}