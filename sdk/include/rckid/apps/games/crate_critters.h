#pragma once

#include <rckid/graphics/canvas.h>
#include <rckid/ui/carousel.h>
#include <rckid/ui/label.h>

#include <assets/games/crate_critters.h>

#include <assets/Baloo64.h>
#include <assets/Iosevka16.h>
#include <assets/OpenDyslexic24.h>

namespace rckid {

    /** Crate Critters, a sokoban clone.

        Push all the crates onto their marks. This is a port of the older Sokoban version I made, renamed so that it does not step on the original's name.
     */
    class CrateCritters : public CanvasApp<void> {
    public:

        String name() const override { return "CrateCritters"; }

        Capabilities capabilities() const override {
            return Capabilities {
                .canPersistState = true,
                .consumesBudget = true,
            };
        }

        static constexpr uint8_t VERSION = 1;

        CrateCritters():
            CanvasApp<void>{},
            imgs_{
                Bitmap{ImageSource{assets::games::crate_critters::wall}},
                Bitmap{ImageSource{assets::games::crate_critters::crate}},
                Bitmap{ImageSource{assets::games::crate_critters::place}},
                Bitmap{ImageSource{assets::games::crate_critters::player}},
            },
            icon_{ImageSource{assets::games::crate_critters::icon}}
        {
            // the carousel is a plain UI widget rendered into our canvas, so it has no parent to size it or to apply the style for it
            levelSelect_.setRect(Rect::WH(CAROUSEL_RECT.width(), CAROUSEL_RECT.height()));
            levelSelect_.applyStyle();
            loadLevelStatus();
            // find the last unlocked level and move towards it
            for (level_ = 0; level_ < NUM_LEVELS; ++level_)
                if (levelStatus_[level_] == LEVEL_UNLOCKED)
                    break;
            setLevel(level_ + 1);
            showLevel(Direction::Up);
            //audio_[1].setEnvelope(100, 50, 80, 250);
        }

    protected:

        void saveState(RandomWriteStream & into) const override {
            into.binaryWriter()
                << VERSION
                << static_cast<uint8_t>(mode_)
                << level_
                << moves_
                << totalMoves_
                << static_cast<int32_t>(player_.x)
                << static_cast<int32_t>(player_.y)
                << wrapBuffer(map_, sizeof(map_));
        }

        bool loadState(RandomReadStream & from) override {
            auto rd = from.binaryReader();
            if (read<uint8_t>(rd) != VERSION) {
                LOG(LL_WARN, "Unsupported save version, skipping");
                return false;
            }
            mode_ = static_cast<Mode>(read<uint8_t>(rd));
            int32_t px;
            int32_t py;
            rd
                >> level_
                >> moves_
                >> totalMoves_
                >> px
                >> py
                >> wrapBuffer(map_, sizeof(map_));
            player_ = Point{px, py};
            showLevel();
            return true;
        }

        void loop() override {
            CanvasApp<void>::loop();
            switch (mode_) {
                case Mode::Intro: {
                    if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                        if (levelUnlocked(level_)) {
                            mode_ = Mode::Game;
                            setLevel(level_);
                            rumbler::nudge();
                        } else {
                            rumbler::fail();
                        }
                    }
                    if (btnPressed(Btn::Left)) {
                        setLevel(level_ - 1);
                        showLevel(Direction::Left);
                        rumbler::nudge();
                    }
                    if (btnPressed(Btn::Right)) {
                        setLevel(level_ + 1);
                        showLevel(Direction::Right);
                        rumbler::nudge();
                    }
                    if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                        btnClear(Btn::B);
                        exit();
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
                        btnClear(Btn::B);
                        showLevel();
                    }
                    // TODO debug only
                    if (btnPressed(Btn::Select))
                        setLevel(level_ + 1);
                    break;
                }
            }
        }

        void render() override {
            // the level selection carousel is a UI widget rendered into our canvas, which means we are responsible for the per frame UI bookkeeping (animations) ourselves
            ui::Widget::renderEssentials(rect());
            canvas().fill(Color::Black());
            switch (mode_) {
                case Mode::Intro: {
                    Font f{assets::Baloo64};
                    Coord tw = f->textWidth(TITLE);
                    canvas().text((320 - tw) / 2, 20, f, Color::Red()) << TITLE;
                    canvas().blit(Point{128, 95}, icon_);
                    levelSelect_.renderOn(canvas(), CAROUSEL_RECT.topLeft());
                    break;
                }
                case Mode::Game: {
                    drawMap();
                    drawPlayer();
                    Font f{assets::OpenDyslexic24};
                    String str{STR(moves_ << " moves")};
                    canvas().text(320 - f->textWidth(str), 218, f, LIGHT_GRAY) << str;
                    str = STR("Level " << level_);
                    canvas().text(0, 218, f, LIGHT_GRAY) << str;
                    break;
                }
            }
            CanvasApp<void>::render();
        }

    private:

        static constexpr char const * TITLE = "Crate Critters";

        static constexpr Coord TILE_WIDTH = 24;
        static constexpr Coord TILE_HEIGHT = 24;
        static constexpr Coord ROWS = 240 / TILE_HEIGHT;
        static constexpr Coord COLS = 320 / TILE_WIDTH;
        static constexpr Coord OFFSETX = (320 - COLS * TILE_WIDTH) / 2;
        static constexpr Coord OFFSETY = (240 - ROWS * TILE_HEIGHT) / 2;

        static constexpr Coord ICON_SIZE = 64;

        /** Where the level selection carousel sits on the screen and how much of it the status line under the level name takes. 
         */
        static constexpr Rect CAROUSEL_RECT = Rect::XYWH(0, 160, 320, 80);
        static constexpr Coord STATUS_HEIGHT = 16;

        static constexpr uint8_t TILE_EMPTY = 0;
        static constexpr uint8_t TILE_WALL = 1;
        static constexpr uint8_t TILE_CRATE = 2;
        static constexpr uint8_t TILE_PLACED_CRATE = 3;
        static constexpr uint8_t TILE_FLOOR = 4;
        static constexpr uint8_t TILE_PLACE = 5;
        static constexpr uint8_t TILE_PLAYER = 9;

        static constexpr uint32_t LEVEL_LOCKED = 0;
        static constexpr uint32_t LEVEL_UNLOCKED = 0xffffffff;

        static inline Color const LIGHT_GRAY = Color::White().withBrightness(192);

        enum class Mode {
            Intro,
            Game,
        };

        /** Shows the currently selected level in the carousel, optionally animating it in from the given direction. 
         
            The level name is the carousel item's own text, the solved / locked status underneath it is a decoration attached to the item's label, the same way the launcher decorates its menu items.
         */
        void showLevel(std::optional<Direction> dir = std::nullopt) {
            String name{STR("Level " << level_)};
            if (dir.has_value())
                levelSelect_.set(std::move(name), levelIcon(level_), dir.value());
            else
                levelSelect_.set(std::move(name), levelIcon(level_));
            using namespace ui;
            Color c = levelUnlocked(level_) ? Color::White() : Color::Gray();
            Label * label = levelSelect_.currentLabel();
            // the item is two lines tall, so the name goes to the top and the status underneath it
            with(label)
                << SetVAlign(VAlign::Top)
                << SetFg(c);
            uint32_t moves = levelStatus_[level_ - 1];
            String status;
            switch (moves) {
                case LEVEL_LOCKED:
                    status = "Locked";
                    break;
                case LEVEL_UNLOCKED:
                    status = "Unlocked";
                    break;
                default:
                    status = STR("Solved in " << moves << " moves");
                    break;
            }
            label->addChild(new Label{})
                << SetRect(Rect::XYWH(0, label->height() - STATUS_HEIGHT, label->width(), STATUS_HEIGHT))
                << SetFont(assets::Iosevka16)
                << SetFg(c)
                << SetHAlign(HAlign::Left)
                << SetVAlign(VAlign::Top)
                << SetText(std::move(status));
        }

        /** Renders a miniature of the currently loaded map to be used as the level's icon in the carousel.
         */
        Bitmap levelIcon(uint32_t level) {
            Bitmap bmp{ICON_SIZE, ICON_SIZE, Color::Representation::RGB565};
            Color w = levelUnlocked(level) ? Color::Red() : Color::Red().withBrightness(128);
            Color c = levelUnlocked(level) ? Color::Yellow() : Color::Black();
            Color p = levelUnlocked(level) ? Color::Blue() : Color::Black();
            fillIcon(bmp, Rect::WH(ICON_SIZE, ICON_SIZE), Color::Black());
            Coord tSize = std::min(ICON_SIZE / COLS, ICON_SIZE / ROWS);
            Coord offsetX = (ICON_SIZE - COLS * tSize) / 2;
            Coord yy = (ICON_SIZE - ROWS * tSize) / 2;
            uint8_t const * tiles = map_;
            for (Coord y = 0; y < ROWS; ++y) {
                Coord xx = offsetX;
                for (Coord x = 0; x < COLS; ++x) {
                    switch (*(tiles++)) {
                        case TILE_WALL:
                            fillIcon(bmp, Rect::XYWH(xx, yy, tSize, tSize), w);
                            break;
                        case TILE_PLACED_CRATE:
                        case TILE_CRATE:
                            fillIcon(bmp, Rect::XYWH(xx, yy, tSize, tSize), c);
                            break;
                        case TILE_PLACE:
                            fillIcon(bmp, Rect::XYWH(xx, yy, tSize, tSize), p);
                            break;
                        default:
                            break;
                    }
                    xx += tSize;
                }
                yy += tSize;
            }
            return bmp;
        }

        static void fillIcon(Bitmap & bmp, Rect rect, Color color) {
            for (Coord x = rect.left(), xe = rect.right(); x < xe; ++x)
                for (Coord y = rect.top(), ye = rect.bottom(); y < ye; ++y)
                    bmp.setPixel(x, y, Color::RGB565{color});
        }

        /** Draws the map.
         */
        void drawMap() {
            uint8_t const * tiles = map_;
            Coord yy = OFFSETY;
            for (Coord y = 0; y < ROWS; ++y) {
                Coord xx = OFFSETX;
                for (Coord x = 0; x < COLS; ++x) {
                    drawTile(xx, yy, *(tiles++));
                    xx += TILE_WIDTH;
                }
                yy += TILE_HEIGHT;
            }
        }

        void drawTile(Coord x, Coord y, uint8_t t) {
            switch (t) {
                default:
                case TILE_EMPTY:
                    return;
                case TILE_WALL:
                    canvas().blit(Point{x, y}, imgs_[0]);
                    return;
                case TILE_FLOOR:
                    // canvas().fill(Rect::XYWH(x, y, TILE_WIDTH, TILE_HEIGHT), Color::DarkGray());
                    return;
                case TILE_PLACE:
                    canvas().blit(Point{x, y}, imgs_[2]);
                    return;
                case TILE_CRATE:
                case TILE_PLACED_CRATE:
                    canvas().blit(Point{x, y}, imgs_[1]);
                    return;
            }
        }

        void drawPlayer() {
            canvas().blit(Point{OFFSETX + player_.x * TILE_WIDTH, OFFSETY + player_.y * TILE_HEIGHT}, imgs_[3]);
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
                    if (changed)
                        saveLevelStatus();
                }
                //audio_[0].setFrequency(50, 20);
            } else {
                //audio_[1].setFrequency(220, 50);
                rumbler::success();
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
                    return true;
                // if there is a crate, we need to check if the crate itself has a place to move
                case TILE_CRATE: {}
                case TILE_PLACED_CRATE: {
                    Point p2 = target + d;
                    uint8_t t2 = getTile(p2);
                    if (t2 == TILE_FLOOR || t2 == TILE_PLACE || t2 == TILE_EMPTY) {
                        setTile(p2, (t2 == TILE_FLOOR || t2 == TILE_EMPTY) ? TILE_CRATE : TILE_PLACED_CRATE);
                        // play nice sound if we help place stuff
                        //if (getTile(p2) == TILE_PLACED_CRATE)
                        //    audio_[1].setFrequency(440, 100);
                        // TODO This changes empty tiles to floors as sokoban walks over them - might want to change when we reintroduce floor tiles, if at all
                        setTile(target, t1 == TILE_PLACED_CRATE ? TILE_PLACE : TILE_FLOOR);
                        player_ = target;
                        rumbler::nudge();
                        return true;
                    }
                    return false;
                }
                default:
                    return false;
            }
        }

        bool areWeDoneYet() {
            for (Coord i = 0; i < COLS * ROWS; ++i)
                if (map_[i] == TILE_CRATE)
                    return false;
            return true;
        }

        uint8_t getTile(Coord x, Coord y) {
            if (x < 0 || x >= COLS)
                return TILE_WALL;
            if (y < 0 || y >= ROWS)
                return TILE_WALL;
            return map_[x + y * COLS];
        }

        void setTile(Coord x, Coord y, uint8_t tile) {
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
            for (Coord y = 0; y < ROWS; ++y) {
                for (Coord x = 0; x < COLS; ++x) {
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

        /** \name Level progress

            The solved/unlocked status of all levels is kept in a small file in the app's home folder. It is deliberately not a part of the app state (which the SDK saves & loads in slots) as it is the progress of the player, not of the current game.
         */
        //@{
        void loadLevelStatus() {
            for (uint32_t i = 0; i < NUM_LEVELS; ++i)
                levelStatus_[i] = LEVEL_LOCKED;
            levelStatus_[0] = LEVEL_UNLOCKED;
            auto f = readFile(LEVELS_FILE);
            if (f == nullptr)
                return;
            auto rd = f->binaryReader();
            if (read<uint8_t>(rd) != VERSION) {
                LOG(LL_WARN, "Unsupported level status version, ignoring");
                return;
            }
            uint32_t n = read<uint32_t>(rd);
            for (uint32_t i = 0; i < n && i < NUM_LEVELS; ++i)
                rd >> levelStatus_[i];
        }

        void saveLevelStatus() {
            auto f = writeFile(LEVELS_FILE);
            if (f == nullptr)
                return;
            auto wr = f->binaryWriter();
            wr << VERSION << static_cast<uint32_t>(NUM_LEVELS);
            for (uint32_t i = 0; i < NUM_LEVELS; ++i)
                wr << levelStatus_[i];
        }
        //@}

        bool levelUnlocked(uint32_t level) {
            return levelStatus_[level - 1] != LEVEL_LOCKED;
        }

        static constexpr char const * LEVELS_FILE = "levels.dat";

        Mode mode_ = Mode::Intro;

        uint32_t level_ = 1;
        uint32_t moves_ = 0;
        uint32_t totalMoves_ = 0;
        Point player_;
        uint8_t map_[COLS * ROWS];

        //ToneGenerator audio_;

        Bitmap imgs_[4];
        Bitmap icon_;

        ui::Carousel levelSelect_;

        static constexpr uint8_t _ = TILE_EMPTY;
        static constexpr uint8_t X = TILE_PLAYER;

        /** Level descriptions

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

        /** Level status - either LEVEL_LOCKED, LEVEL_UNLOCKED, or the number of moves the level has been solved in.
         */
        uint32_t levelStatus_[NUM_LEVELS];

    }; // rckid::CrateCritters

} // namespace rckid
