#pragma once

#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/ui/header.h>
#include <rckid/ui/alert.h>
#include <rckid/ui/carousel.h>
#include <rckid/filesystem.h>

#include <rckid/assets/fonts/OpenDyslexic24.h>
#include <rckid/assets/fonts/OpenDyslexic48.h>
#include <rckid/assets/fonts/HemiHead64.h>
#include <rckid/assets/icons24.h>
#include <rckid/assets/icons64.h>


namespace rckid {

    /** Sokoban clone
     
     */
    class Sokoban : public GraphicsApp<Canvas<ColorRGB>> {
    public:
        
        static void run() {
            Sokoban g;
            g.loop();
        }

    protected:

        class SokobanLevel : public MenuItem {
        public:
            SokobanLevel(Sokoban * parent) {
                setPayloadPtr(parent);
                setPayload(parent->level_);
            }

            void text(std::string & text) const override { 
                text = STR("Level " << payload());
            }


            bool icon(Bitmap<ColorRGB> &bmp) const override {
                Sokoban & parent = * reinterpret_cast<Sokoban*>(payloadPtr());
                uint32_t level = payload();
                ColorRGB w = parent.levelUnlocked(level) ? color::Red : color::DarkRed;
                ColorRGB c = parent.levelUnlocked(level) ? color::Yellow : color::Black;
                ColorRGB p = parent.levelUnlocked(level) ? color::Blue : color::Black;

                bmp.fill(color::Black);
                int tSize = std::min(bmp.width() / COLS, bmp.height() / ROWS); 
                int offsetX = (bmp.width() - COLS * tSize) / 2;
                int yy = (bmp.height() - ROWS * tSize) / 2;
                uint8_t * tiles = parent.map_;
                for (int y = 0; y < ROWS; ++y) {
                    int xx = offsetX;
                    for (int x = 0; x < COLS; ++x) {
                        switch ((*tiles++)) {
                            case TILE_WALL:
                                bmp.fill(w, Rect::XYWH(xx, yy, tSize, tSize));
                                break;
                            case TILE_PLACED_CRATE:
                            case TILE_CRATE:
                                bmp.fill(c, Rect::XYWH(xx, yy, tSize, tSize));
                                break;
                            case TILE_PLACE:
                                bmp.fill(p, Rect::XYWH(xx, yy, tSize, tSize));
                                break;
                            default:
                                break;
                        }
                        xx += tSize;
                    }
                    yy += tSize;
                }
                return true;
            }
        }; 

        class SokobanCarousel : public Carousel {
        public:
            using Carousel::Carousel;
        protected:
            void drawText(Bitmap<ColorRGB> & surface, int x, int y, Item & item) override {
                Sokoban & parent = * reinterpret_cast<Sokoban*>(item.payloadPtr);
                uint32_t level = item.payload;
                ColorRGB c = parent.levelUnlocked(level) ? color::White : color::Gray;
                // TODO does nothing interesting - eventually might scroll the text if too large to fit, etc
                surface.text(x, y - 8, font(), c) << item.text;
                uint32_t moves = parent.levelStatus_[level - 1];
                switch (moves) {
                    case LEVEL_LOCKED:
                        surface.text(x, y + 32, assets::font::Iosevka16::font, c) << "Locked";
                        break;
                    case LEVEL_UNLOCKED:
                        surface.text(x, y + 32, assets::font::Iosevka16::font, c) << "Unlocked";
                        break;
                    default:
                        surface.text(x, y + 32, assets::font::Iosevka16::font, c) << "Solved in " << moves << " moves";
                        break;
                }
            }
        };

        Sokoban(): GraphicsApp{ARENA(Canvas<Color>{320, 240})} {
            imgs_[0].loadImage(PNG::fromBuffer(assets::icons24::wall));
            imgs_[1].loadImage(PNG::fromBuffer(assets::icons24::wooden_box));
            imgs_[2].loadImage(PNG::fromBuffer(assets::icons24::gps));
            imgs_[3].loadImage(PNG::fromBuffer(assets::icons24::boy));
            icon_.loadImage(PNG::fromBuffer(assets::icons64::wooden_box));
            loadLevelStatus();
            setLevel(1);
            levelSelect_.setCurrent(SokobanLevel{this});
        }

        void update() override {
            switch (mode_) {
                case Mode::Intro: {
                    if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                        if (levelUnlocked(level_)) {
                            mode_ = Mode::Game;
                            setLevel(level_);
                        } else {
                            rumbleFail();
                        }
                    }
                    if (btnPressed(Btn::Left)) {
                        setLevel(level_ - 1);
                        levelSelect_.moveLeft(SokobanLevel{this});
                    }
                    if (btnPressed(Btn::Right)) {
                        setLevel(level_ + 1);
                        levelSelect_.moveRight(SokobanLevel{this});
                    }
                    break;
                }
                case Mode::Game: {
                    if (btnPressed(Btn::Up))
                        tryMove(Point{0, -1});
                    if (btnPressed(Btn::Down))
                        tryMove(Point{0, 1});
                    if (btnPressed(Btn::Left))
                        tryMove(Point{-1, 0});
                    if (btnPressed(Btn::Right))
                        tryMove(Point{1, 0});
                    if (btnPressed(Btn::Start))
                        setLevel(level_);
                    if (btnPressed(Btn::B)) {
                        mode_ = Mode::Intro;
                        btnPressedClear(Btn::B);
                        levelSelect_.setCurrent(SokobanLevel{this});
                    }
                    // TODO debug only
                    if (btnPressed(Btn::Select))
                        setLevel(level_ + 1);
                    break;
                }
                default:
                    // no use control in other modes
                    break;
            }
            GraphicsApp::update();
        }

        void draw() override {
            NewArenaScope _{};
            g_.fill();
            switch (mode_) {
                case Mode::Intro: {
                    g_.text(40, 20, assets::font::HemiHead64::font, color::Red) << "Sokoban";
                    g_.blit(Point{128, 95}, icon_);
                    levelSelect_.drawOn(g_, Rect::XYWH(0, 160, 320, 80));
                    break;
                }
                case Mode::Game: {
                    drawMap();
                    drawPlayer();

                    std::string str{STR(moves_ << " moves")};
                    g_.text(320 - assets::font::OpenDyslexic24::font.textWidth(str), 218, assets::font::OpenDyslexic24::font, color::LightGray) << str;
                    str = STR("Level " << level_);
                    g_.text(0, 218, assets::font::OpenDyslexic24::font, color::LightGray) << str;
                    break;
                } 
                default:
                    UNIMPLEMENTED;
            }
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
                    bool changed = false;
                    // do we have high score? 
                    if (levelStatus_[level_ - 1] > moves_) {
                        levelStatus_[level_ - 1] = moves_;
                        changed = true;
                    }
                    // unlock next level 
                    setLevel(level_ + 1);
                    if (levelStatus_[level_ - 1] == LEVEL_LOCKED) {
                        levelStatus_[level_ - 1] = LEVEL_UNLOCKED;
                        changed = true;
                    }
                    // if changed, update the table 
                    if (changed) {
                        using namespace filesystem;
                        FileWrite f = fileWrite("sokoban.levels", Drive::Cartridge);
                        if (f.good()) {
                            // very dirty serialization that does not check stuff at all
                            // TODO my future self will fix it
                            for (unsigned i = 0; i < NUM_LEVELS; ++i)
                                f.serialize<uint32_t>(levelStatus_[i]);
                        }
                    }
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
                case TILE_EMPTY:
                    player_ = target;
                    rumbleNudge();
                    return true;
                // if there is a crate, we need to check if the crate itself has a place to move
                case TILE_CRATE: {}
                case TILE_PLACED_CRATE: {
                    Point p2 = target + d;
                    uint8_t t2 = getTile(p2);
                    if (t2 == TILE_FLOOR || t2 == TILE_PLACE || t2 == TILE_EMPTY) {
                        setTile(p2, (t2 == TILE_FLOOR || t2 == TILE_EMPTY) ? TILE_CRATE : TILE_PLACED_CRATE);
                        // TODO This changes empty tiles to floors as sokoban walks over them - might want to change when we reintroduce floor tiles, if at all
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

        void setLevel(uint32_t value) {
            if (value > NUM_LEVELS)
                value = 1;
            else if (value == 0)
                value = NUM_LEVELS;
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

        void loadLevelStatus() {
            using namespace filesystem;
            FileRead f = fileRead("sokoban.levels", Drive::Cartridge);
            if (f.good()) {
                // very dirty deserialization that does not check stuff at all
                // TODO my future self will fix it
                for (unsigned i = 0; i < NUM_LEVELS; ++i)
                    levelStatus_[i] = f.deserialize<uint32_t>();
            } else {
                for (unsigned i = 0; i < NUM_LEVELS; ++i)
                    levelStatus_[i] = LEVEL_LOCKED;
                levelStatus_[0] = LEVEL_UNLOCKED;    
            }
        }

        bool levelUnlocked(uint32_t level) {
            return levelStatus_[level - 1] != LEVEL_LOCKED;
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

        enum class Mode {
            Intro, 
            Game, 
            Reset,
            Done,
        }; 

        Mode mode_ = Mode::Intro;

        SokobanCarousel levelSelect_{assets::font::OpenDyslexic48::font};

        uint32_t level_ = 1;
        uint32_t moves_;
        uint32_t totalMoves_;
        Point player_;
        uint8_t map_[COLS * ROWS];

        Bitmap<ColorRGB> icon_{64, 64};

        Bitmap<ColorRGB> imgs_[4] = {
            Bitmap<ColorRGB>{24, 24},
            Bitmap<ColorRGB>{24, 24},
            Bitmap<ColorRGB>{24, 24},
            Bitmap<ColorRGB>{24, 24},
        };

        static constexpr uint8_t _ = TILE_EMPTY;
        static constexpr uint8_t X = TILE_PLAYER;

        /** Sokoban levels description

            This is copied from http://www.sneezingtiger.com/sokoban/levelpics/microbanImages.html (text version http://www.sneezingtiger.com/sokoban/levels/microbanText.html) as the levels are quite small and yet challenging for the target demographics. Below is python script used to convert the tiles from the ASCII representation on the page to the C++ notation used here:

                def parse_levels(input_string):
                    levels = {}
                    current_level = []
                    level_name = None
                    
                    for line in input_string.split('\n'):
                        line = line.rstrip()
                        if line.startswith('Level'):
                            if current_level:
                                levels[level_name] = current_level
                                current_level = []
                            level_name = line
                        elif line or level_name:
                            current_level.append(line)
                    
                    if current_level:
                        levels[level_name] = current_level
                    
                    return levels

                def transform_sokoban_level(level):
                    symbol_map = {
                        ' ': '_',
                        '#': '1',
                        '@': 'X',
                        '$': '2',
                        '.': '5',
                        '*': '3'
                    }
                    
                    grid = [['_' for _ in range(13)] for _ in range(10)]
                    start_row = (10 - len(level)) // 2
                    start_col = (13 - max(len(row) for row in level)) // 2
                    for i, row in enumerate(level):
                        for j, char in enumerate(row):
                            if char in symbol_map:
                                grid[start_row + i][start_col + j] = symbol_map[char]
                    
                    return grid

                def print_grid(level_name, grid):
                    print('{ // ' + level_name)
                    for row in grid:
                        print('    ', ','.join(row), ',')
                    print('},')

                # Example input string
                input_string = """
                Level 11
                ######
                #    #
                # ##@##
                ### # $ #
                # ..# $ #
                #       #
                #  ######
                ####
                """

                # Parse the levels from the input string
                levels = parse_levels(input_string)

                # Transform and print the levels
                for level_name, level in levels.items():
                    if len(level) <= 10 and max(len(row) for row in level) <= 13:
                        grid = transform_sokoban_level(level)
                        print_grid(level_name, grid)
                    else:
                        print(f"{level_name} is too large to fit in the 13x10 grid and will be skipped.")

         */
        static constexpr uint8_t levels_[][COLS * ROWS] = {
            { // level 1
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,1,1,1,1,1,_,_,_,_,
                _,_,1,1,1,_,_,_,1,_,_,_,_,
                _,_,1,_,_,2,_,_,1,1,1,_,_,
                _,_,1,_,_,5,_,_,_,_,1,_,_,
                _,_,1,1,_,_,_,_,_,_,1,_,_,
                _,_,_,1,_,_,_,_,_,X,1,_,_,
                _,_,_,1,1,1,1,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            {
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,1,1,_,_,_,_,
                _,_,_,1,_,_,_,_,1,_,_,_,_,
                _,_,_,1,_,1,X,_,1,_,_,_,_,
                _,_,_,1,_,2,3,_,1,_,_,_,_,
                _,_,_,1,_,5,3,_,1,_,_,_,_,
                _,_,_,1,_,_,_,_,1,_,_,_,_,
                _,_,_,1,1,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // level 3
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,1,1,1,1,_,_,_,_,_,
                _,_,1,1,1,_,_,1,1,1,1,_,_,
                _,_,1,_,_,_,_,_,2,_,1,_,_,
                _,_,1,_,1,_,_,1,2,_,1,_,_,
                _,_,1,_,5,_,5,1,X,_,1,_,_,
                _,_,1,1,1,1,1,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // level 4
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,1,1,1,1,1,1,1,1,_,_,_,
                _,_,1,_,_,_,_,_,_,1,_,_,_,
                _,_,1,_,5,3,3,2,X,1,_,_,_,
                _,_,1,_,_,_,_,_,_,1,_,_,_,
                _,_,1,1,1,1,1,_,_,1,_,_,_,
                _,_,_,_,_,_,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // level 5
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,1,1,1,_,_,_,
                _,_,_,1,_,_,_,_,_,1,_,_,_,
                _,_,_,1,_,5,2,5,_,1,_,_,_,
                _,_,1,1,_,2,X,2,_,1,_,_,_,
                _,_,1,_,_,5,2,5,_,1,_,_,_,
                _,_,1,_,_,_,_,_,_,1,_,_,_,
                _,_,1,1,1,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // level 6
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,1,1,1,1,1,1,_,1,1,1,1,1,
                _,1,_,_,_,_,1,1,1,_,_,_,1,
                _,1,_,2,2,_,_,_,_,_,1,X,1,
                _,1,_,2,_,1,5,5,5,_,_,_,1,
                _,1,_,_,_,1,1,1,1,1,1,1,1,
                _,1,1,1,1,1,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // level 7
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,1,1,1,_,_,_,
                _,_,_,1,_,_,_,_,_,1,_,_,_,
                _,_,_,1,_,5,2,5,_,1,_,_,_,
                _,_,_,1,_,2,5,2,_,1,_,_,_,
                _,_,_,1,_,5,2,5,_,1,_,_,_,
                _,_,_,1,_,2,5,2,_,1,_,_,_,
                _,_,_,1,_,_,X,_,_,1,_,_,_,
                _,_,_,1,1,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // level 8 is too high 
            { // level 9 
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,1,1,1,1,1,_,_,_,_,_,_,
                _,_,1,5,_,_,1,1,_,_,_,_,_,
                _,_,1,X,2,2,_,1,_,_,_,_,_,
                _,_,1,1,_,_,_,1,_,_,_,_,_,
                _,_,_,1,1,_,_,1,_,_,_,_,_,
                _,_,_,_,1,1,5,1,_,_,_,_,_,
                _,_,_,_,_,1,1,1,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 11
                _,_,_,_,1,1,1,1,1,1,_,_,_,
                _,_,_,_,1,_,_,_,_,1,_,_,_,
                _,_,_,_,1,_,1,1,X,1,1,_,_,
                _,_,1,1,1,_,1,_,2,_,1,_,_,
                _,_,1,_,5,5,1,_,2,_,1,_,_,
                _,_,1,_,_,_,_,_,_,_,1,_,_,
                _,_,1,_,_,1,1,1,1,1,1,_,_,
                _,_,1,1,1,1,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 12
                _,_,1,1,1,1,1,_,_,_,_,_,_,
                _,_,1,_,_,_,1,1,_,_,_,_,_,
                _,_,1,_,2,_,_,1,_,_,_,_,_,
                _,_,1,1,_,2,_,1,1,1,1,_,_,
                _,_,_,1,1,1,X,5,_,_,1,_,_,
                _,_,_,_,1,_,_,5,1,_,1,_,_,
                _,_,_,_,1,_,_,_,_,_,1,_,_,
                _,_,_,_,1,1,1,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 13
                _,_,_,1,1,1,1,_,_,_,_,_,_,
                _,_,_,1,5,_,1,1,_,_,_,_,_,
                _,_,_,1,5,X,_,1,_,_,_,_,_,
                _,_,_,1,5,_,2,1,_,_,_,_,_,
                _,_,_,1,1,2,_,1,1,1,_,_,_,
                _,_,_,_,1,_,2,_,_,1,_,_,_,
                _,_,_,_,1,_,_,_,_,1,_,_,_,
                _,_,_,_,1,_,_,1,1,1,_,_,_,
                _,_,_,_,1,1,1,1,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 14
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,1,1,1,_,_,_,
                _,_,_,1,_,_,_,_,_,1,_,_,_,
                _,_,_,1,_,1,_,1,_,1,_,_,_,
                _,_,_,1,5,_,2,3,X,1,_,_,_,
                _,_,_,1,_,_,_,1,1,1,_,_,_,
                _,_,_,1,1,1,1,1,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 15
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,1,1,1,_,_,_,
                _,_,1,1,1,1,1,1,X,1,1,_,_,
                _,_,1,_,_,_,_,5,3,_,1,_,_,
                _,_,1,_,_,_,1,_,_,_,1,_,_,
                _,_,1,1,1,1,1,2,1,_,1,_,_,
                _,_,_,_,_,_,1,_,_,_,1,_,_,
                _,_,_,_,_,_,1,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 16
                _,_,1,1,1,1,_,_,_,_,_,_,_,
                _,_,1,_,_,1,1,1,1,_,_,_,_,
                _,_,1,_,_,_,_,_,1,1,_,_,_,
                _,1,1,_,1,1,_,_,_,1,_,_,_,
                _,1,5,_,5,1,_,X,2,1,1,_,_,
                _,1,_,_,_,1,_,2,2,_,1,_,_,
                _,1,_,_,5,1,_,_,_,_,1,_,_,
                _,1,1,1,1,1,1,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 17
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,1,_,_,_,_,_,
                _,_,_,1,_,X,_,1,_,_,_,_,_,
                _,_,_,1,5,5,5,1,_,_,_,_,_,
                _,_,_,1,2,2,2,1,1,_,_,_,_,
                _,_,_,1,_,_,_,_,1,_,_,_,_,
                _,_,_,1,_,_,_,_,1,_,_,_,_,
                _,_,_,1,1,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 18
                _,_,_,1,1,1,1,1,1,1,_,_,_,
                _,_,_,1,_,_,_,_,_,1,_,_,_,
                _,_,_,1,5,_,5,_,_,1,_,_,_,
                _,_,_,1,_,1,1,_,1,1,_,_,_,
                _,_,_,1,_,_,2,_,1,_,_,_,_,
                _,_,_,1,1,1,2,_,1,_,_,_,_,
                _,_,_,_,_,1,X,_,1,_,_,_,_,
                _,_,_,_,_,1,_,_,1,_,_,_,_,
                _,_,_,_,_,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 19
                _,_,1,1,1,1,1,1,1,1,_,_,_,
                _,_,1,_,_,_,5,5,_,1,_,_,_,
                _,_,1,_,_,X,2,2,_,1,_,_,_,
                _,_,1,1,1,1,1,_,1,1,_,_,_,
                _,_,_,_,_,1,_,_,1,_,_,_,_,
                _,_,_,_,_,1,_,_,1,_,_,_,_,
                _,_,_,_,_,1,_,_,1,_,_,_,_,
                _,_,_,_,_,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 20
                _,_,1,1,1,1,1,1,1,_,_,_,_,
                _,_,1,_,_,_,_,_,1,1,1,_,_,
                _,_,1,_,_,X,2,2,5,5,1,_,_,
                _,_,1,1,1,1,_,1,1,_,1,_,_,
                _,_,_,_,1,_,_,_,_,_,1,_,_,
                _,_,_,_,1,_,_,1,1,1,1,_,_,
                _,_,_,_,1,_,_,1,_,_,_,_,_,
                _,_,_,_,1,1,1,1,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 21
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,_,_,_,_,_,_,
                _,_,_,1,_,_,1,1,1,1,_,_,_,
                _,_,_,1,_,5,_,5,_,1,_,_,_,
                _,_,_,1,_,2,2,1,X,1,_,_,_,
                _,_,_,1,1,_,_,_,_,1,_,_,_,
                _,_,_,_,1,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 22
                _,_,_,1,1,1,1,1,_,_,_,_,_,
                _,_,_,1,_,_,_,1,1,1,_,_,_,
                _,_,_,1,5,_,5,_,_,1,_,_,_,
                _,_,_,1,_,_,_,1,_,1,_,_,_,
                _,_,_,1,1,_,1,_,_,1,_,_,_,
                _,_,_,_,1,X,2,2,_,1,_,_,_,
                _,_,_,_,1,_,_,_,_,1,_,_,_,
                _,_,_,_,1,_,_,1,1,1,_,_,_,
                _,_,_,_,1,1,1,1,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 23
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,1,1,1,_,_,_,
                _,_,_,1,_,_,3,_,_,1,_,_,_,
                _,_,_,1,_,_,_,_,_,1,_,_,_,
                _,_,_,1,1,_,1,_,1,1,_,_,_,
                _,_,_,_,1,2,X,5,1,_,_,_,_,
                _,_,_,_,1,_,_,_,1,_,_,_,_,
                _,_,_,_,1,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 24
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,_,1,1,1,1,1,_,_,_,
                _,_,_,_,_,1,_,_,_,1,_,_,_,
                _,_,_,1,1,1,2,2,X,1,_,_,_,
                _,_,_,1,_,_,_,1,1,1,_,_,_,
                _,_,_,1,_,_,_,_,_,1,_,_,_,
                _,_,_,1,_,5,_,5,_,1,_,_,_,
                _,_,_,1,1,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 25
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,1,1,1,1,_,_,_,_,_,
                _,_,_,_,1,_,_,1,1,1,_,_,_,
                _,_,_,_,1,_,2,2,_,1,_,_,_,
                _,_,_,1,1,5,5,5,_,1,_,_,_,
                _,_,_,1,_,_,X,2,_,1,_,_,_,
                _,_,_,1,_,_,_,1,1,1,_,_,_,
                _,_,_,1,1,1,1,1,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 26
                _,_,_,_,1,1,1,1,1,_,_,_,_,
                _,_,_,_,1,_,X,_,1,_,_,_,_,
                _,_,_,_,1,_,_,_,1,_,_,_,_,
                _,_,_,1,1,1,2,_,1,_,_,_,_,
                _,_,_,1,_,5,5,5,1,_,_,_,_,
                _,_,_,1,_,2,2,_,1,_,_,_,_,
                _,_,_,1,1,1,_,_,1,_,_,_,_,
                _,_,_,_,_,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 27
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,1,1,_,_,_,_,
                _,_,_,1,_,_,_,5,1,_,_,_,_,
                _,_,_,1,_,1,1,_,1,1,_,_,_,
                _,_,_,1,_,_,2,2,X,1,_,_,_,
                _,_,_,1,_,1,_,_,_,1,_,_,_,
                _,_,_,1,5,_,_,1,1,1,_,_,_,
                _,_,_,1,1,1,1,1,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 28
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,1,_,_,_,_,_,
                _,_,_,1,_,_,_,1,_,_,_,_,_,
                _,_,_,1,_,X,_,1,_,_,_,_,_,
                _,_,_,1,_,2,2,1,1,1,_,_,_,
                _,_,_,1,1,5,_,5,_,1,_,_,_,
                _,_,_,_,1,_,_,_,_,1,_,_,_,
                _,_,_,_,1,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 29
                _,_,_,_,_,_,1,1,1,1,1,_,_,
                _,_,_,_,_,_,1,_,_,_,1,1,_,
                _,_,_,_,_,_,1,_,_,_,_,1,_,
                _,_,1,1,1,1,1,1,_,_,_,1,_,
                _,1,1,_,_,_,_,_,1,5,_,1,_,
                _,1,_,2,_,2,_,X,_,_,1,1,_,
                _,1,_,1,1,1,1,1,1,5,1,_,_,
                _,1,_,_,_,_,_,_,_,_,1,_,_,
                _,1,1,1,1,1,1,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 30
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,_,_,_,_,_,_,
                _,_,_,1,_,_,1,1,1,_,_,_,_,
                _,_,_,1,_,2,2,_,1,_,_,_,_,
                _,_,_,1,5,5,5,_,1,_,_,_,_,
                _,_,_,1,_,X,2,_,1,_,_,_,_,
                _,_,_,1,_,_,_,1,1,_,_,_,_,
                _,_,_,1,1,1,1,1,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 31
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,1,1,1,1,_,_,_,_,
                _,_,_,_,1,1,_,_,1,_,_,_,_,
                _,_,_,1,1,X,2,5,1,1,_,_,_,
                _,_,_,1,_,2,2,_,_,1,_,_,_,
                _,_,_,1,_,5,_,5,_,1,_,_,_,
                _,_,_,1,1,1,_,_,_,1,_,_,_,
                _,_,_,_,_,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 32
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,1,1,1,1,_,_,_,_,_,
                _,_,_,1,1,_,_,1,1,1,_,_,_,
                _,_,_,1,_,_,_,_,_,1,_,_,_,
                _,_,_,1,5,3,3,2,X,1,_,_,_,
                _,_,_,1,_,_,_,1,1,1,_,_,_,
                _,_,_,1,1,_,_,1,_,_,_,_,_,
                _,_,_,_,1,1,1,1,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 33
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,1,1,1,_,_,_,
                _,_,_,1,5,_,1,_,_,1,_,_,_,
                _,_,_,1,_,_,2,_,_,1,_,_,_,
                _,_,_,1,5,_,2,1,X,1,_,_,_,
                _,_,_,1,_,_,2,_,_,1,_,_,_,
                _,_,_,1,5,_,1,_,_,1,_,_,_,
                _,_,_,1,1,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 34
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,1,1,1,1,_,_,_,_,_,
                _,_,1,1,1,_,_,1,1,1,1,_,_,
                _,_,1,_,_,_,_,_,_,_,1,_,_,
                _,_,1,X,2,3,3,3,5,_,1,_,_,
                _,_,1,_,_,_,_,_,_,_,1,_,_,
                _,_,1,1,1,1,1,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 35 is too large to fit in the 13x10 grid and will be skipped.
            // Level 36 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 37
                _,_,_,_,_,_,_,_,1,1,1,_,_,
                _,_,1,1,1,1,1,_,1,5,1,_,_,
                _,_,1,_,_,_,1,1,1,5,1,_,_,
                _,_,1,_,_,_,2,_,1,5,1,_,_,
                _,_,1,_,2,_,_,2,_,_,1,_,_,
                _,_,1,1,1,1,1,X,1,_,1,_,_,
                _,_,_,_,_,_,1,_,_,_,1,_,_,
                _,_,_,_,_,_,1,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 38
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,1,1,1,1,1,1,1,1,1,1,_,_,
                _,1,_,_,_,_,_,_,_,_,1,_,_,
                _,1,_,1,1,5,1,1,1,_,1,_,_,
                _,1,_,1,_,2,2,_,5,_,1,_,_,
                _,1,_,5,_,X,2,1,1,_,1,_,_,
                _,1,1,1,1,1,_,_,_,_,1,_,_,
                _,_,_,_,_,1,1,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 39
                _,1,1,1,1,1,_,_,_,_,_,_,_,
                _,1,_,_,_,1,1,1,1,_,_,_,_,
                _,1,_,1,_,1,_,5,1,_,_,_,_,
                _,1,_,_,_,_,2,_,1,1,1,_,_,
                _,1,1,1,_,1,2,5,_,_,1,_,_,
                _,1,_,_,_,1,X,_,_,_,1,_,_,
                _,1,_,1,_,1,1,1,1,1,1,_,_,
                _,1,_,_,_,1,_,_,_,_,_,_,_,
                _,1,1,1,1,1,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            /* -- can't have player on place
            { // Level 40
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,1,1,1,1,1,_,_,_,_,
                _,_,_,_,1,_,_,_,1,_,_,_,_,
                _,_,_,1,1,_,_,_,1,1,_,_,_,
                _,_,_,1,_,2,2,2,_,1,_,_,_,
                _,_,_,1,_,5,_,5,_,1,_,_,_,
                _,_,_,1,1,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            */
            { // Level 41
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,1,1,1,1,1,1,1,_,_,_,_,
                _,_,1,_,_,_,_,_,1,_,_,_,_,
                _,_,1,X,2,2,2,_,1,1,_,_,_,
                _,_,1,_,_,1,5,5,5,1,_,_,_,
                _,_,1,1,_,_,_,_,1,1,_,_,_,
                _,_,_,1,1,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 42
                _,_,_,_,_,_,1,1,1,1,_,_,_,
                _,_,_,_,_,_,1,_,_,1,_,_,_,
                _,_,_,_,_,_,1,X,_,1,_,_,_,
                _,_,_,1,1,1,1,2,5,1,_,_,_,
                _,_,_,1,_,_,_,2,5,1,_,_,_,
                _,_,_,1,_,1,_,2,5,1,_,_,_,
                _,_,_,1,_,_,_,_,1,1,_,_,_,
                _,_,_,1,1,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 43
                _,_,_,_,_,_,_,1,1,1,1,_,_,
                _,_,_,_,_,_,_,1,_,X,1,_,_,
                _,_,_,_,_,_,_,1,_,_,1,_,_,
                _,_,1,1,1,1,1,1,_,5,1,_,_,
                _,_,1,_,_,_,2,_,_,5,1,_,_,
                _,_,1,_,_,2,2,1,_,5,1,_,_,
                _,_,1,_,_,_,_,1,1,1,1,_,_,
                _,_,1,1,1,_,_,1,_,_,_,_,_,
                _,_,_,_,1,1,1,1,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 44
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,1,_,_,_,_,_,
                _,_,_,1,X,2,5,1,_,_,_,_,_,
                _,_,_,1,1,1,1,1,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 45
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,1,1,_,_,_,_,
                _,_,_,1,5,5,5,_,1,_,_,_,_,
                _,_,_,1,_,_,2,_,1,_,_,_,_,
                _,_,_,1,_,1,2,1,1,_,_,_,_,
                _,_,_,1,_,_,2,_,1,_,_,_,_,
                _,_,_,1,_,_,X,_,1,_,_,_,_,
                _,_,_,1,1,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 46
                _,_,_,_,1,1,1,1,1,1,_,_,_,
                _,_,_,1,1,_,_,_,_,1,_,_,_,
                _,_,_,1,_,_,1,1,_,1,_,_,_,
                _,_,_,1,_,1,_,2,_,1,_,_,_,
                _,_,_,1,_,_,3,_,5,1,_,_,_,
                _,_,_,1,1,_,1,X,1,1,_,_,_,
                _,_,_,_,1,_,_,_,1,_,_,_,_,
                _,_,_,_,1,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 47
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,1,1,1,_,_,_,
                _,1,1,1,_,_,_,_,_,1,_,_,_,
                _,1,_,2,_,2,_,_,_,1,_,_,_,
                _,1,_,1,1,1,_,1,1,1,1,1,_,
                _,1,_,X,_,5,_,5,_,_,_,1,_,
                _,1,_,_,_,1,1,1,_,_,_,1,_,
                _,1,1,1,1,1,_,1,1,1,1,1,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 48
                _,_,1,1,1,1,1,1,_,_,_,_,_,
                _,_,1,_,_,X,_,1,_,_,_,_,_,
                _,_,1,_,_,1,_,1,1,_,_,_,_,
                _,_,1,_,5,1,_,_,1,1,_,_,_,
                _,_,1,_,5,2,2,2,_,1,_,_,_,
                _,_,1,_,5,1,_,_,_,1,_,_,_,
                _,_,1,1,1,1,_,_,_,1,_,_,_,
                _,_,_,_,_,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 49 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 50
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,_,_,_,_,_,_,
                _,1,1,1,_,_,1,1,1,1,1,_,_,
                _,1,_,_,2,_,_,X,5,5,1,_,_,
                _,1,_,2,_,_,_,_,1,_,1,_,_,
                _,1,1,1,_,1,1,1,1,_,1,_,_,
                _,_,_,1,_,_,_,_,_,_,1,_,_,
                _,_,_,1,1,1,1,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 51
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,1,1,1,1,_,_,_,_,_,_,_,
                _,_,1,_,_,1,1,1,_,_,_,_,_,
                _,_,1,_,_,_,_,1,1,1,_,_,_,
                _,_,1,_,_,2,3,X,_,1,_,_,_,
                _,_,1,1,1,_,5,1,_,1,_,_,_,
                _,_,_,_,1,_,_,_,_,1,_,_,_,
                _,_,_,_,1,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 52
                _,_,_,_,_,1,1,1,1,_,_,_,_,
                _,_,_,1,1,1,_,X,1,_,_,_,_,
                _,_,_,1,_,_,2,_,1,_,_,_,_,
                _,_,_,1,_,_,3,5,1,_,_,_,_,
                _,_,_,1,_,_,3,5,1,_,_,_,_,
                _,_,_,1,_,_,2,_,1,_,_,_,_,
                _,_,_,1,1,1,_,_,1,_,_,_,_,
                _,_,_,_,_,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 53
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,1,1,1,1,1,_,_,_,_,
                _,_,_,1,1,5,_,5,1,1,_,_,_,
                _,_,_,1,_,3,_,3,_,1,_,_,_,
                _,_,_,1,_,_,1,_,_,1,_,_,_,
                _,_,_,1,_,2,_,2,_,1,_,_,_,
                _,_,_,1,1,_,X,_,1,1,_,_,_,
                _,_,_,_,1,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 54
                _,_,_,_,_,_,1,1,1,1,1,1,_,
                _,_,_,_,_,_,1,_,_,_,_,1,_,
                _,_,1,1,1,1,1,_,5,_,_,1,_,
                1,1,1,_,_,1,1,1,5,_,_,1,_,
                1,_,2,_,_,2,_,_,5,_,1,1,_,
                1,_,X,2,2,_,1,_,5,_,1,_,_,
                1,1,_,_,_,_,1,1,1,1,1,_,_,
                _,1,1,1,1,1,1,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 55
                _,1,1,1,1,1,1,1,1,_,_,_,_,
                _,1,_,X,_,1,_,_,1,_,_,_,_,
                _,1,_,_,_,_,_,_,1,_,_,_,_,
                _,1,1,1,1,1,2,_,1,_,_,_,_,
                _,_,_,_,_,1,_,_,1,1,1,_,_,
                _,_,1,1,_,1,2,_,5,5,1,_,_,
                _,_,1,1,_,1,_,_,1,1,1,_,_,
                _,_,_,_,_,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 56
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,1,_,_,_,_,_,
                _,_,_,1,_,_,_,1,1,1,_,_,_,
                _,_,_,1,_,_,2,_,_,1,_,_,_,
                _,_,_,1,1,3,_,5,_,1,_,_,_,
                _,_,_,_,1,_,_,_,X,1,_,_,_,
                _,_,_,_,1,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 57
                _,_,_,_,1,1,1,1,_,_,_,_,_,
                _,_,_,_,1,_,_,1,_,_,_,_,_,
                _,_,_,_,1,X,_,1,_,_,_,_,_,
                _,_,_,_,1,_,_,1,_,_,_,_,_,
                _,_,1,1,1,_,1,1,1,1,_,_,_,
                _,_,1,_,_,_,_,3,_,1,_,_,_,
                _,_,1,_,_,2,_,_,_,1,_,_,_,
                _,_,1,1,1,1,1,5,_,1,_,_,_,
                _,_,_,_,_,_,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 58
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,_,_,_,_,_,_,
                _,_,_,1,_,_,1,1,1,1,_,_,_,
                _,_,_,1,5,3,2,_,_,1,_,_,_,
                _,_,_,1,_,5,2,1,_,1,_,_,_,
                _,_,_,1,1,_,X,_,_,1,_,_,_,
                _,_,_,_,1,_,_,_,1,1,_,_,_,
                _,_,_,_,1,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 59
                1,1,1,1,1,1,1,1,1,1,1,1,_,
                1,_,_,_,_,_,_,_,_,_,_,1,_,
                1,_,1,1,1,1,1,1,1,_,X,1,1,
                1,_,1,_,_,_,_,_,_,_,_,_,1,
                1,_,1,_,_,2,_,_,_,1,_,_,1,
                1,_,2,2,_,1,1,1,1,1,_,_,1,
                1,1,1,_,_,1,_,1,_,5,5,5,1,
                _,_,1,1,1,1,_,1,_,_,_,_,1,
                _,_,_,_,_,_,_,1,1,1,1,1,1,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 60 is too large to fit in the 13x10 grid and will be skipped.
            // Level 61 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 62
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,1,1,1,1,1,1,1,1,1,1,
                1,1,1,1,_,_,_,_,1,1,_,_,1,
                1,_,_,2,2,2,5,5,5,5,2,X,1,
                1,_,_,_,_,_,_,1,1,1,_,_,1,
                1,_,_,_,1,1,1,1,_,1,1,1,1,
                1,1,1,1,1,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 63 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 64
                _,_,1,1,1,1,1,1,_,_,_,_,_,
                _,1,1,_,_,_,_,1,_,_,_,_,_,
                _,1,_,_,_,2,_,1,_,_,_,_,_,
                _,1,_,_,2,2,_,1,_,_,_,_,_,
                _,1,1,1,_,5,1,1,1,1,1,_,_,
                _,_,_,1,1,5,1,_,X,_,1,_,_,
                _,_,_,_,1,5,_,_,2,_,1,_,_,
                _,_,_,_,1,5,_,1,1,1,1,_,_,
                _,_,_,_,1,1,1,1,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 65
                _,_,_,_,1,1,1,1,1,1,_,_,_,
                _,_,_,_,1,_,_,_,_,1,_,_,_,
                _,_,_,_,1,_,_,2,_,1,_,_,_,
                _,_,_,1,1,1,1,2,_,1,_,_,_,
                _,_,1,1,_,2,_,2,_,1,_,_,_,
                _,_,1,5,5,5,5,1,_,1,1,_,_,
                _,_,1,_,_,_,_,_,X,_,1,_,_,
                _,_,1,1,_,_,1,_,_,_,1,_,_,
                _,_,_,1,1,1,1,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 66 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 67
                _,_,_,1,1,1,1,1,_,_,_,_,_,
                _,_,_,1,_,_,_,1,1,_,_,_,_,
                _,_,_,1,_,1,_,_,1,_,_,_,_,
                _,_,_,1,X,2,3,5,1,1,_,_,_,
                _,_,_,1,1,_,_,5,_,1,_,_,_,
                _,_,_,_,1,_,2,1,_,1,_,_,_,
                _,_,_,_,1,1,_,_,_,1,_,_,_,
                _,_,_,_,_,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 68
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,1,1,1,1,_,_,_,_,_,_,_,
                _,_,1,_,_,1,1,1,1,1,1,_,_,
                _,1,1,_,_,_,_,2,_,_,1,_,_,
                _,1,_,5,1,_,2,_,_,_,1,_,_,
                _,1,_,5,1,2,1,1,1,1,1,_,_,
                _,1,_,5,X,_,1,_,_,_,_,_,_,
                _,1,1,1,1,1,1,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 69
                _,1,1,1,1,_,_,1,1,1,1,_,_,
                _,1,_,_,1,1,1,1,_,_,1,_,_,
                _,1,_,_,1,_,_,1,_,_,1,_,_,
                _,1,_,_,1,_,_,_,_,2,1,1,_,
                _,1,_,_,5,_,5,1,2,_,_,1,_,
                _,1,X,_,1,1,_,1,_,2,_,1,_,
                _,1,_,_,_,5,_,1,_,_,_,1,_,
                _,1,1,1,1,1,1,1,1,1,1,1,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 70 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 71
                1,1,1,1,1,1,1,1,1,1,1,_,_,
                1,_,_,_,_,_,1,_,_,_,1,1,1,
                1,_,2,X,2,_,1,_,5,_,_,5,1,
                1,_,1,1,_,1,1,1,_,1,1,_,1,
                1,_,1,_,_,_,_,_,_,_,1,_,1,
                1,_,1,_,_,_,1,_,_,_,1,_,1,
                1,_,1,1,1,1,1,1,1,1,1,_,1,
                1,_,_,_,_,_,_,_,_,_,_,_,1,
                1,1,1,1,1,1,1,1,1,1,1,1,1,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 72 is too large to fit in the 13x10 grid and will be skipped.
            // Level 73 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 74
                _,_,1,1,1,1,_,_,_,_,_,_,_,
                _,_,1,_,_,1,1,1,1,1,1,1,_,
                _,_,1,2,_,X,1,_,_,_,5,1,_,
                _,1,1,_,1,2,2,_,_,_,5,1,_,
                _,1,_,_,2,_,_,1,1,5,5,1,_,
                _,1,_,_,_,1,_,1,1,1,1,1,_,
                _,1,1,1,_,_,_,1,_,_,_,_,_,
                _,_,_,1,1,1,1,1,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 75
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,1,1,1,1,1,1,1,_,_,_,_,
                _,1,1,_,5,5,5,5,1,1,_,_,_,
                _,1,_,_,_,1,1,1,1,1,1,_,_,
                _,1,_,_,_,2,_,2,_,X,1,_,_,
                _,1,1,1,_,_,2,_,2,_,1,_,_,
                _,_,_,1,1,1,_,_,_,_,1,_,_,
                _,_,_,_,_,1,1,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 76 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 77
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,1,1,1,1,1,1,1,1,1,1,_,_,
                _,1,_,X,_,5,5,5,5,_,1,_,_,
                _,1,_,_,_,1,1,1,1,2,1,1,_,
                _,1,1,_,1,_,_,2,_,2,_,1,_,
                _,_,1,_,2,_,_,_,_,_,_,1,_,
                _,_,1,_,_,_,1,1,1,1,1,1,_,
                _,_,1,1,1,1,1,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 78
                _,_,1,1,1,1,1,1,1,_,_,_,_,
                _,1,1,_,_,_,_,_,1,1,_,_,_,
                _,1,_,_,2,_,2,_,_,1,_,_,_,
                _,1,_,2,_,2,_,2,_,1,_,_,_,
                _,1,1,_,1,1,1,_,1,1,1,1,_,
                _,_,1,X,_,_,5,5,5,5,5,1,_,
                _,_,1,1,_,_,_,_,_,1,1,1,_,
                _,_,_,1,1,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 79
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,1,1,1,1,1,1,1,1,1,_,_,
                _,_,1,_,_,_,_,1,_,_,1,_,_,
                _,1,1,_,2,1,2,1,_,_,1,_,_,
                _,1,_,_,5,2,5,X,_,_,1,_,_,
                _,1,_,_,5,1,_,_,_,_,1,_,_,
                _,1,1,1,1,1,1,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 80 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 81
                _,_,_,_,1,1,1,1,1,_,_,_,_,
                _,_,_,_,1,_,_,_,1,_,_,_,_,
                _,_,_,_,1,_,5,_,1,_,_,_,_,
                _,_,_,1,1,_,3,_,1,_,_,_,_,
                _,_,_,1,_,_,3,1,1,_,_,_,_,
                _,_,_,1,_,_,X,1,1,_,_,_,_,
                _,_,_,1,1,_,2,_,1,_,_,_,_,
                _,_,_,_,1,_,_,_,1,_,_,_,_,
                _,_,_,_,1,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 82
                _,_,1,1,1,1,1,_,_,_,_,_,_,
                _,_,1,_,_,_,1,1,1,_,_,_,_,
                _,_,1,_,5,_,_,_,1,1,_,_,_,
                _,_,1,1,3,1,2,_,_,1,_,_,_,
                _,_,1,_,5,1,_,2,_,1,_,_,_,
                _,_,1,_,X,1,1,_,1,1,_,_,_,
                _,_,1,_,_,_,_,_,1,_,_,_,_,
                _,_,1,1,1,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 83 is too large to fit in the 13x10 grid and will be skipped.
            // Level 84 is too large to fit in the 13x10 grid and will be skipped.
            // Level 85 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 86
                _,_,_,_,_,_,1,1,1,1,_,_,_,
                _,_,_,_,1,1,1,_,_,1,1,_,_,
                _,_,_,1,1,_,2,_,_,_,1,_,_,
                _,_,1,1,_,2,_,_,1,_,1,_,_,
                _,_,1,_,X,1,2,2,_,_,1,_,_,
                _,_,1,_,5,5,_,_,1,1,1,_,_,
                _,_,1,_,5,5,1,1,1,_,_,_,_,
                _,_,1,1,1,1,1,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 87 is too large to fit in the 13x10 grid and will be skipped.
            // Level 88 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 89
                _,1,1,1,1,1,1,1,1,1,1,_,_,
                _,1,_,_,_,1,1,_,_,_,1,_,_,
                _,1,_,2,_,_,2,X,1,_,1,_,_,
                _,1,1,1,1,_,1,_,2,_,1,_,_,
                _,_,_,_,1,5,1,_,_,1,1,_,_,
                _,_,1,_,1,5,1,_,2,1,_,_,_,
                _,_,1,_,1,5,_,_,_,1,_,_,_,
                _,_,1,_,1,5,_,_,_,1,_,_,_,
                _,_,_,_,1,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 90
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,1,1,1,1,1,1,1,1,_,_,_,
                _,_,1,_,_,X,_,_,_,1,_,_,_,
                _,_,1,_,2,_,_,2,_,1,_,_,_,
                _,1,1,1,_,1,1,_,1,1,1,_,_,
                _,1,_,_,2,5,5,2,_,_,1,_,_,
                _,1,_,_,_,5,5,_,_,_,1,_,_,
                _,1,1,1,1,1,1,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 91
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,1,1,1,1,1,1,1,1,1,1,1,_,
                _,1,_,_,_,_,5,1,1,_,_,1,_,
                _,1,_,2,2,X,5,5,2,2,_,1,_,
                _,1,_,_,_,1,1,5,_,_,_,1,_,
                _,1,1,1,1,1,1,1,1,1,1,1,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 92 is too large to fit in the 13x10 grid and will be skipped.
            // Level 93 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 94
                _,_,1,1,1,1,1,1,1,1,1,_,_,
                _,_,1,_,X,_,1,_,_,_,1,_,_,
                _,_,1,_,2,_,2,_,_,_,1,_,_,
                _,_,1,1,2,1,1,1,_,1,1,_,_,
                _,_,1,_,_,5,5,5,_,_,1,_,_,
                _,_,1,_,_,_,1,_,_,_,1,_,_,
                _,_,1,1,1,1,1,1,_,_,1,_,_,
                _,_,_,_,_,_,_,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 95
                _,_,1,1,1,1,1,1,1,1,_,_,_,
                _,_,1,X,_,_,_,_,_,1,_,_,_,
                _,_,1,_,5,2,2,5,_,1,_,_,_,
                _,_,1,_,2,5,5,2,_,1,_,_,_,
                _,_,1,_,2,5,5,2,_,1,_,_,_,
                _,_,1,_,5,2,2,5,_,1,_,_,_,
                _,_,1,_,_,_,_,_,_,1,_,_,_,
                _,_,1,1,1,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 96 is too large to fit in the 13x10 grid and will be skipped.
            // Level 97 is too large to fit in the 13x10 grid and will be skipped.
            // Level 98 is too large to fit in the 13x10 grid and will be skipped.
            // Level 99 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 100
                _,_,1,1,1,1,1,1,1,_,_,_,_,
                _,_,1,_,X,1,_,_,1,_,_,_,_,
                _,_,1,5,2,_,_,_,1,_,_,_,_,
                _,_,1,5,_,1,_,2,1,1,_,_,_,
                _,_,1,5,2,1,_,_,_,1,_,_,_,
                _,_,1,5,_,1,_,2,_,1,_,_,_,
                _,_,1,_,_,1,_,_,_,1,_,_,_,
                _,_,1,1,1,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 101 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 102
                _,1,1,1,1,1,1,1,1,1,1,1,_,
                _,1,5,5,5,5,1,_,_,_,_,1,_,
                _,1,_,_,1,_,_,_,2,2,_,1,_,
                _,1,_,_,X,_,_,1,1,_,_,1,_,
                _,1,_,_,_,_,_,1,1,2,_,1,_,
                _,1,1,1,1,1,1,_,_,2,_,1,_,
                _,_,_,_,_,_,1,_,_,_,_,1,_,
                _,_,_,_,_,_,1,1,1,1,1,1,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 103
                _,_,_,_,1,1,1,1,1,_,_,_,_,
                _,_,_,_,1,_,5,_,1,1,_,_,_,
                _,_,1,1,1,_,2,_,_,1,_,_,_,
                _,_,1,_,5,_,2,1,X,1,_,_,_,
                _,_,1,_,1,2,_,5,_,1,_,_,_,
                _,_,1,_,_,2,_,1,1,1,_,_,_,
                _,_,1,1,_,5,_,1,_,_,_,_,_,
                _,_,_,1,1,1,1,1,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 104
                _,_,_,_,_,_,1,1,1,1,1,_,_,
                _,_,1,1,1,1,1,_,_,_,1,_,_,
                _,_,1,_,_,_,_,2,_,_,1,_,_,
                _,_,1,_,_,2,1,2,1,X,1,_,_,
                _,_,1,1,1,_,1,_,_,_,1,_,_,
                _,_,_,_,1,_,5,5,5,_,1,_,_,
                _,_,_,_,1,1,1,_,_,1,1,_,_,
                _,_,_,_,_,_,1,_,_,1,_,_,_,
                _,_,_,_,_,_,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 105 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 106
                _,_,1,1,1,1,1,1,1,1,_,_,_,
                _,_,1,_,_,_,_,_,_,1,_,_,_,
                _,_,1,X,_,_,_,2,_,1,_,_,_,
                _,1,1,_,1,1,1,2,_,1,_,_,_,
                _,1,_,5,5,5,5,5,1,1,1,_,_,
                _,1,_,2,_,2,_,2,_,_,1,_,_,
                _,1,1,1,1,1,1,_,1,_,1,_,_,
                _,_,_,_,_,_,1,_,_,_,1,_,_,
                _,_,_,_,_,_,1,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 107
                _,_,1,1,1,1,1,1,1,1,_,_,_,
                _,_,1,_,_,_,_,_,_,1,_,_,_,
                _,_,1,_,2,3,3,3,_,1,_,_,_,
                _,_,1,_,3,_,_,3,_,1,_,_,_,
                _,_,1,_,3,_,_,3,_,1,_,_,_,
                _,_,1,_,3,3,3,5,_,1,_,_,_,
                _,_,1,_,_,_,_,_,X,1,_,_,_,
                _,_,1,1,1,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 108 is too large to fit in the 13x10 grid and will be skipped.
            // Level 109 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 110
                _,_,_,_,1,1,1,1,_,_,_,_,_,
                _,_,_,_,1,_,_,1,_,_,_,_,_,
                _,_,_,_,1,_,2,1,1,1,1,_,_,
                _,_,1,1,1,5,_,5,_,_,1,_,_,
                _,_,1,_,2,_,1,_,2,_,1,_,_,
                _,_,1,_,_,5,_,5,1,1,1,_,_,
                _,_,1,1,1,1,2,_,1,_,_,_,_,
                _,_,_,_,_,1,_,X,1,_,_,_,_,
                _,_,_,_,_,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 111 is too large to fit in the 13x10 grid and will be skipped.
            // Level 112 is too large to fit in the 13x10 grid and will be skipped.
            // Level 113 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 114
                _,1,1,1,1,1,1,_,_,_,_,_,_,
                _,1,_,_,_,_,1,1,1,_,_,_,_,
                _,1,_,_,1,_,2,_,1,_,_,_,_,
                _,1,_,_,2,_,X,_,1,_,_,_,_,
                _,1,1,_,1,1,_,1,1,1,1,1,_,
                _,1,_,_,1,5,5,5,5,5,5,1,_,
                _,1,_,2,_,2,_,2,_,2,_,1,_,
                _,1,1,_,_,_,1,1,1,1,1,1,_,
                _,_,1,1,1,1,1,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 115
                _,_,_,_,1,1,1,1,1,_,_,_,_,
                1,1,1,1,1,_,_,_,1,1,1,1,_,
                1,_,_,_,_,_,1,_,_,_,_,1,_,
                1,_,_,1,5,5,5,5,5,_,_,1,_,
                1,1,_,_,1,1,_,1,_,1,1,1,_,
                _,1,2,2,X,2,2,2,_,1,_,_,_,
                _,1,_,_,_,_,_,1,1,1,_,_,_,
                _,1,1,1,1,1,1,1,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 116
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,1,1,1,1,1,_,_,
                _,_,_,_,1,1,1,_,_,_,1,_,_,
                _,1,1,1,1,5,5,5,5,5,1,_,_,
                _,1,_,X,2,2,2,2,2,_,1,_,_,
                _,1,_,_,_,_,_,1,_,1,1,_,_,
                _,1,1,1,1,1,_,_,_,1,_,_,_,
                _,_,_,_,_,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 117 is too large to fit in the 13x10 grid and will be skipped.
            // Level 118 is too large to fit in the 13x10 grid and will be skipped.
            // Level 119 is too large to fit in the 13x10 grid and will be skipped.
            // Level 120 is too large to fit in the 13x10 grid and will be skipped.
            // Level 121 is too large to fit in the 13x10 grid and will be skipped.
            // Level 122 is too large to fit in the 13x10 grid and will be skipped.
            // Level 123 is too large to fit in the 13x10 grid and will be skipped.
            // Level 124 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 125
                _,_,_,_,_,_,1,1,1,1,1,_,_,
                _,_,_,_,_,_,1,_,_,_,1,1,_,
                _,_,_,_,_,_,1,_,2,_,_,1,_,
                1,1,1,1,1,1,1,1,_,1,X,1,1,
                1,_,5,_,_,1,_,2,_,2,_,_,1,
                1,_,_,_,_,_,_,_,_,2,1,_,1,
                1,5,5,5,1,1,1,1,1,_,_,_,1,
                1,1,1,1,1,_,_,_,1,1,1,1,1,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 126
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,1,1,1,1,1,1,1,1,1,1,1,_,
                1,1,5,5,5,5,5,5,5,_,_,1,_,
                1,_,2,2,2,2,2,2,2,X,_,1,_,
                1,_,_,_,1,_,1,_,1,_,1,1,_,
                1,_,1,_,1,_,_,_,_,_,1,_,_,
                1,_,_,_,1,1,1,1,1,1,1,_,_,
                1,1,1,1,1,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 127
                _,1,1,_,1,1,1,1,_,_,_,_,_,
                _,1,1,1,1,_,_,1,1,1,1,_,_,
                _,_,1,_,2,_,2,5,_,_,1,_,_,
                _,1,1,_,1,_,_,5,2,_,1,_,_,
                _,1,_,_,_,1,1,5,1,1,1,_,_,
                _,1,_,_,2,_,_,5,_,1,_,_,_,
                _,1,_,X,_,1,_,_,_,1,_,_,_,
                _,1,_,_,1,1,1,1,1,1,_,_,_,
                _,1,1,1,1,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 128
                _,_,_,1,1,1,1,1,1,1,1,1,_,
                _,1,1,1,_,_,_,1,_,_,_,1,_,
                _,1,_,3,_,2,_,5,_,5,_,1,_,
                _,1,_,_,_,2,_,1,1,_,1,1,_,
                _,1,1,1,1,3,1,_,_,_,1,_,_,
                _,_,1,_,_,X,_,_,1,1,1,_,_,
                _,_,1,_,_,_,1,1,1,_,_,_,_,
                _,_,1,1,1,1,1,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 129
                _,_,_,1,1,1,1,1,1,1,1,1,_,
                _,1,1,1,_,X,_,1,_,_,_,1,_,
                _,1,_,3,_,2,_,3,5,5,_,1,_,
                _,1,_,_,_,2,_,1,_,_,_,1,_,
                _,1,1,1,1,3,1,_,_,1,1,1,_,
                _,_,1,_,_,_,_,_,1,1,_,_,_,
                _,_,1,_,_,_,1,1,1,_,_,_,_,
                _,_,1,1,1,1,1,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 130
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                1,1,1,1,1,_,_,1,1,1,1,1,_,
                1,_,_,_,1,1,1,1,5,5,_,1,_,
                1,_,2,2,2,_,_,_,_,_,_,1,_,
                1,_,_,_,2,1,_,_,5,5,_,1,_,
                1,1,1,_,X,1,_,_,1,1,_,1,_,
                _,_,1,_,_,1,1,_,_,_,_,1,_,
                _,_,1,1,1,1,1,1,1,1,1,1,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 131 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 132
                _,1,1,1,1,_,_,_,_,_,_,_,_,
                _,1,_,X,1,1,1,_,_,_,_,_,_,
                _,1,5,3,_,_,1,1,1,1,1,_,_,
                _,1,5,5,1,2,2,_,2,_,1,_,_,
                _,1,1,_,_,_,_,_,_,_,1,_,_,
                _,_,1,_,1,_,1,1,_,_,1,_,_,
                _,_,1,_,_,_,1,1,1,1,1,_,_,
                _,_,1,1,1,1,1,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 133
                _,_,1,1,1,1,1,1,1,_,_,_,_,
                _,_,1,_,_,5,_,5,1,1,1,_,_,
                _,_,1,_,5,_,5,_,5,_,1,_,_,
                _,1,1,1,_,1,1,1,1,_,1,_,_,
                _,1,_,_,X,2,_,_,2,_,1,_,_,
                _,1,_,_,2,2,_,_,2,_,1,_,_,
                _,1,1,1,1,_,_,_,1,1,1,_,_,
                _,_,_,_,1,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 134 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 135
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,1,1,1,1,1,1,1,_,_,_,_,_,
                _,1,_,_,_,_,_,1,1,1,1,1,_,
                _,1,_,2,2,1,X,1,1,5,5,1,_,
                _,1,_,1,_,_,_,_,_,_,_,1,_,
                _,1,_,_,2,_,1,_,1,_,_,1,_,
                _,1,1,1,1,_,2,_,_,5,5,1,_,
                _,_,_,_,1,1,1,1,1,1,1,1,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 136
                _,_,_,1,1,1,1,1,1,1,_,_,_,
                _,_,_,1,_,_,_,_,_,1,_,_,_,
                _,_,1,1,_,1,1,1,2,1,1,_,_,
                _,_,1,5,2,_,_,_,X,_,1,_,_,
                _,_,1,_,5,5,_,1,2,_,1,_,_,
                _,_,1,5,1,1,_,_,2,_,1,_,_,
                _,_,1,_,_,_,_,1,1,1,1,_,_,
                _,_,1,1,1,1,1,1,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 137 is too large to fit in the 13x10 grid and will be skipped.
            // Level 138 is too large to fit in the 13x10 grid and will be skipped.
            // Level 139 is too large to fit in the 13x10 grid and will be skipped.
            // Level 140 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 141
                _,_,_,1,1,1,1,1,1,1,1,_,_,
                _,_,_,1,_,_,1,_,5,_,1,_,_,
                _,_,_,1,_,_,_,5,3,5,1,_,_,
                _,_,_,1,_,_,1,_,3,_,1,_,_,
                _,1,1,1,1,2,1,1,5,1,1,_,_,
                _,1,_,_,_,_,_,_,2,_,1,_,_,
                _,1,_,2,_,1,1,_,2,_,1,_,_,
                _,1,_,_,_,X,1,_,_,_,1,_,_,
                _,1,1,1,1,1,1,1,1,1,1,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            { // Level 142
                _,_,_,_,1,1,1,1,_,_,_,_,_,
                _,_,_,_,1,_,_,1,_,_,_,_,_,
                _,_,_,_,1,_,_,1,1,1,1,_,_,
                _,_,1,1,1,2,5,2,_,_,1,_,_,
                _,_,1,_,_,5,X,5,_,_,1,_,_,
                _,_,1,_,_,2,5,2,1,1,1,_,_,
                _,_,1,1,1,1,_,_,1,_,_,_,_,
                _,_,_,_,_,1,_,_,1,_,_,_,_,
                _,_,_,_,_,1,1,1,1,_,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 143 is too large to fit in the 13x10 grid and will be skipped.
            // Level 144 is too large to fit in the 13x10 grid and will be skipped.
            // Level 145 is too large to fit in the 13x10 grid and will be skipped.
            { // Level 146
                _,_,_,1,1,1,1,1,1,1,_,_,_,
                _,_,1,1,_,_,5,_,_,1,1,_,_,
                _,_,1,_,5,2,2,2,5,_,1,_,_,
                _,_,1,_,2,5,_,5,2,_,1,_,_,
                _,_,1,5,2,_,X,_,2,5,1,_,_,
                _,_,1,_,2,5,_,5,2,_,1,_,_,
                _,_,1,_,5,2,2,2,5,_,1,_,_,
                _,_,1,1,_,_,5,_,_,1,1,_,_,
                _,_,_,1,1,1,1,1,1,1,_,_,_,
                _,_,_,_,_,_,_,_,_,_,_,_,_,
            },
            // Level 147 is too large to fit in the 13x10 grid and will be skipped.
            // Level 148 is too large to fit in the 13x10 grid and will be skipped.
            // Level 149 is too large to fit in the 13x10 grid and will be skipped.
            // Level 150 is too large to fit in the 13x10 grid and will be skipped.
            // Level 151 is too large to fit in the 13x10 grid and will be skipped.
            // Level 152 is too large to fit in the 13x10 grid and will be skipped.
            // Level 153 is too large to fit in the 13x10 grid and will be skipped.
        };

        static constexpr uint32_t NUM_LEVELS = sizeof(levels_) / (COLS * ROWS);

        static constexpr uint32_t LEVEL_LOCKED = 0;
        static constexpr uint32_t LEVEL_UNLOCKED = std::numeric_limits<uint32_t>::max();

        /** level status 
         */
        uint32_t levelStatus_[NUM_LEVELS];


    }; // rckid::Sokoban

} // namespace rckid