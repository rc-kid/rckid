#pragma once

#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/ui/header.h>
#include <rckid/ui/alert.h>

#include <rckid/assets/fonts/OpenDyslexic24.h>
#include <rckid/assets/icons24.h>

namespace rckid {

    /** Sokoban clone
     
        - add nice graphics
     */
    class Sokoban : public GraphicsApp<Canvas<ColorRGB>> {
    public:
        
        static void run() {
            Sokoban g;
            g.resetGame();
            g.loop();
        }

    protected:
        Sokoban(): GraphicsApp{Canvas<Color>{320, 240}} {
            imgs_[0].loadImage(PNG::fromBuffer(assets::icons24::wall));
            imgs_[1].loadImage(PNG::fromBuffer(assets::icons24::wooden_box));
            imgs_[2].loadImage(PNG::fromBuffer(assets::icons24::gps));
            imgs_[3].loadImage(PNG::fromBuffer(assets::icons24::boy));
        }

        void update() override {
            GraphicsApp::update();
            if (btnPressed(Btn::Up))
                tryMove(Point{0, -1});
            if (btnPressed(Btn::Down))
                tryMove(Point{0, 1});
            if (btnPressed(Btn::Left))
                tryMove(Point{-1, 0});
            if (btnPressed(Btn::Right))
                tryMove(Point{1, 0});
        }

        void draw() override {
            g_.fill();
            drawMap();
            drawPlayer();

            std::string str{STR(moves_ << " moves")};
            g_.text(320 - assets::font::OpenDyslexic24::font.textWidth(str), 218, assets::font::OpenDyslexic24::font, color::LightGray) << str;
            str = STR("Level " << level_);
            g_.text(0, 218, assets::font::OpenDyslexic24::font, color::LightGray) <<str;
        }



        /** Draws the sokoban map. 
         
            Returns true if the game has been finished, i.e. everything is in place. 
         */
        void drawMap() {
            uint8_t * tiles = map_;
            int yy = OFFSETY;
            for (int y = 0; y < ROWS; ++y) {
                int xx = OFFSETX;
                for (int x = 0; x < COLS; ++x) {
                    drawTile(xx, yy, *(tiles++));
                    xx += TILE_WIDTH;
                }
                yy += TILE_HEIGHT;
            }
        }

        void drawTile(int x, int y, uint8_t t) {
            ColorRGB c;
            switch (t) {
                default:
                case TILE_EMPTY:
                    return;
                case TILE_WALL:
                    g_.blit(Point{x, y}, imgs_[0]);
                    return;
                    break;
                case TILE_FLOOR:
                    c = color::DarkGray;
                    break;
                case TILE_PLACE:
                    g_.blit(Point{x, y}, imgs_[2]);
                    return;
                case TILE_CRATE:
                case TILE_PLACED_CRATE:
                    g_.blit(Point{x, y}, imgs_[1]);
                    return;
            }
            //g_.fill(c, Rect::XYWH(x, y, TILE_WIDTH, TILE_HEIGHT));
        }

        void drawPlayer() {
            g_.blit(Point{OFFSETX + player_.x * TILE_WIDTH, OFFSETY + player_.y * TILE_HEIGHT}, imgs_[3]);
//            g_.fill(color::Green, Rect::XYWH(OFFSETX + player_.x * TILE_WIDTH, OFFSETY + player_.y * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT));
        }

        void tryMove(Point d) {
            if (move(d)) {
                ++moves_;
                if (areWeDoneYet()) {
                    LOG("Level cleared!");
                }
            } else {
                rumbleFail();
            }
        }

        bool move(Point d) {
            Point target = player_ + d;
            uint8_t t1 = getTile(target);
            switch (t1) {
                // if the position we move to is empty, we can always move
                case TILE_FLOOR:
                case TILE_PLACE:
                    player_ = target;
                    rumbleNudge();
                    return true;
                // if there is a crate, we need to check if the crate itself has a place to move
                case TILE_CRATE: {}
                case TILE_PLACED_CRATE: {
                    Point p2 = target + d;
                    uint8_t t2 = getTile(p2);
                    if (t2 == TILE_FLOOR || t2 == TILE_PLACE) {
                        setTile(p2, t2 == TILE_FLOOR ? TILE_CRATE : TILE_PLACED_CRATE);
                        setTile(target, t1 == TILE_PLACED_CRATE ? TILE_PLACE : TILE_FLOOR);
                        player_ = target;
                        rumbleOk();
                        return true;        
                    }
                    return false;
                }
                default:
                    return false;
            }
        }

        bool areWeDoneYet() {
            for (int i = 0; i < COLS * ROWS; ++i)
                if (map_[i] == TILE_CRATE)
                    return false;
            return true;
        }

        uint8_t getTile(int x, int y) {
            if (x < 0 || x >= COLS)
                return TILE_WALL;
            if (y < 0 || y >= ROWS)
                return TILE_WALL;
            return map_[x + y * COLS]; 
        }

        void setTile(int x, int y, uint8_t tile) {
            ASSERT(x >= 0 && x < COLS);
            ASSERT(y >= 0 && y < ROWS);
            map_[x + y * COLS] = tile;
        }

        uint8_t getTile(Point p) { return getTile(p.x, p.y); }
        void setTile(Point p, uint8_t tile) { setTile(p.x, p.y, tile); }

        void resetGame() {
            totalMoves_ = 0;
            moves_ = 0;
            setLevel(1);
        }

        void resetLevel() {
            moves_ = 0;
            setLevel(level_);
        }

        void setLevel(uint32_t value) {
            // copy the level map
            memcpy(map_, levels_[value - 1], sizeof(map_));
            totalMoves_ += moves_;
            level_ = value;
            moves_ = 0;
            // find player's initial position on the map and replace it with floor
            uint8_t * tiles = map_;
            for (int y = 0; y < ROWS; ++y) {
                for (int x = 0; x < COLS; ++x) {
                    if (*tiles == TILE_PLAYER) {
                        player_ = Point{x, y};
                        *tiles = TILE_FLOOR;
                        return;
                    }
                    ++tiles;
                }
            }
            UNREACHABLE;
        }

        static constexpr int TILE_WIDTH = 24;
        static constexpr int TILE_HEIGHT = 24;
        static constexpr int ROWS = 240 / TILE_HEIGHT;
        static constexpr int COLS = 320 / TILE_WIDTH;
        static constexpr int OFFSETX = (320 - COLS * TILE_WIDTH) / 2;
        static constexpr int OFFSETY = (240 - ROWS * TILE_HEIGHT) / 2;



        static constexpr uint8_t TILE_EMPTY = 0;
        static constexpr uint8_t TILE_WALL = 1;
        static constexpr uint8_t TILE_CRATE = 2;
        static constexpr uint8_t TILE_PLACED_CRATE = 3;
        static constexpr uint8_t TILE_FLOOR = 4;
        static constexpr uint8_t TILE_PLACE = 5;
        static constexpr uint8_t TILE_PLAYER = 9;

        uint32_t level_;
        uint32_t moves_;
        uint32_t totalMoves_;
        Point player_;
        uint8_t map_[COLS * ROWS];

        Bitmap<ColorRGB> imgs_[4] = {
            Bitmap<ColorRGB>{24, 24},
            Bitmap<ColorRGB>{24, 24},
            Bitmap<ColorRGB>{24, 24},
            Bitmap<ColorRGB>{24, 24},
        };

        static constexpr uint8_t levels_[][COLS * ROWS] = {
            {
                0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,1,1,1,1,1,0,0,0,0,
                0,0,1,1,1,4,4,4,1,0,0,0,0,
                0,0,1,4,4,2,4,4,1,1,1,0,0,
                0,0,1,4,4,5,4,4,4,4,1,0,0,
                0,0,1,1,4,4,4,4,4,4,1,0,0,
                0,0,0,1,4,4,4,4,4,9,1,0,0,
                0,0,0,1,1,1,1,1,1,1,1,0,0,
                0,0,0,0,0,0,0,0,0,0,0,0,0,
            },
        };

    }; // rckid::Sokoban

} // namespace rckid