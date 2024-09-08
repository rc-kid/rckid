#pragma once

#include "../graphics/color.h"
#include "../graphics/tile.h"
#include "../graphics/sprite.h"
#include "../graphics/drawing.h"
#include "../assets/tiles/ui-tiles.h"

namespace rckid {

    struct UITileInfo {
        char c = ' ';
        uint8_t paletteOffset = 0;

        UITileInfo & operator = (char c) { this->c = c; return *this; }
    }; // UITileInfo

    template<typename TILE = Tile<12, 24, Color16>>
    class UITileEngine : public PaletteHolder<typename TILE::Color> {
    public:

        using Tile = TILE;
        using Color = typename TILE::Color;
        using Sprite = rckid::Sprite<typename TILE::Color>;

        UITileEngine(Coord width, Coord height, Tile const * tileset, ColorRGB const * palette):
            PaletteHolder<Color>{palette},
            tiles_{tileset},
            w_{width}, 
            h_{height},
            tileMap_{ new UITileInfo[width * height] } {
            ASSERT(width > 0 && pixelWidth() <= 320);
            ASSERT(height > 0 && pixelHeight() <= 240);
            top_ = 235 - pixelHeight();
        }

        UITileEngine(UITileEngine const &) = delete;

        UITileEngine(UITileEngine && from):
            PaletteHolder<Color>{from},
            tiles_{from.tiles_}, 
            w_{from.w_}, 
            h_{from.h_}, 
            top_{from.top_},
            tileMap_{from.tileMap_},
            sprites_{std::move(from.sprites_)} 
        {
            from.tileMap_ = nullptr;
        }

        ~UITileEngine() {
            for (Sprite * s : sprites_)
                delete s;
            delete [] tileMap_;
        }

        Tile const * tiles() const { return tiles_; }
        void setTiles(Tile const * tiles) { tiles_ = tiles; }

        Coord width() const { return w_; }
        Coord height() const { return h_; }

        Coord pixelWidth() const { return w_ * Tile::Width; }
        Coord pixelHeight() const { return h_ * Tile::Height; }

        Coord top() const { return top_; }

        size_t numSprites() const { return sprites_.size(); }

        Sprite & addSprite(int width, int height) {
            Sprite * s = new Sprite{width, height};
            sprites_.push_back(s);
            return *s;
        }

        Sprite & getSprite(size_t index) { return *sprites_[index]; }

        UITileInfo const & at(int x, int y) const { return tileMap_[y + x * h_]; }
        UITileInfo const & at(Point p) const { return at(p.x, p.y); }
        UITileInfo & at(int x, int y) { return tileMap_[y + x * h_]; }
        UITileInfo & at(Point p) { return at(p.x, p.y); }

        /** Fills the entire tileset with given value. 
         */
        void fill(char c, uint8_t paletteOffset = 0) {
            for (int i = 0, e = w_ * h_; i != e; ++i)
                tileMap_[i] = UITileInfo{c, paletteOffset};
        }

        /** Single-line writer interface. 
         */
        Writer text(int x, int y, uint8_t paletteOffset = 0) {
            return Writer{[this, x, y, paletteOffset](char c) mutable {
                at(x++, y) = UITileInfo{c, paletteOffset};
            }};
        }

    private:

        friend class Renderer<UITileEngine<TILE>>;

        Tile const * tiles_ = nullptr;

        Coord w_;
        Coord h_;
        Coord top_;
        UITileInfo * tileMap_ = nullptr;
        std::vector<Sprite *> sprites_;

    }; 

    /** Renderer for the simple tile engine.  
     
        Simply renders the single tile layer column by column and overlays any sprites. 
     */
    template<typename TILE>
    class Renderer<UITileEngine<TILE>> {
    public:

        ~Renderer() {
            ASSERT(renderBuffer_ == nullptr);
        }

        void initialize(UITileEngine<TILE> const & te) {
            displaySetMode(DisplayMode::Native);
            displaySetUpdateRegion(Rect::XYWH((320 - te.pixelWidth()) / 2, te.top(), te.pixelWidth(), te.pixelHeight()));
            // create the buffer
            ASSERT(renderBuffer_ == nullptr);
            renderBuffer_ = new ColorRGB[te.pixelHeight() * 2];
        }

        void finalize() {
            ASSERT(renderBuffer_ != nullptr);
            delete [] renderBuffer_;
            renderBuffer_ = nullptr;
        }

        void render(UITileEngine<TILE> const & te) {
            column_ = te.pixelWidth() - 1;
            renderColumn(column_, te);
            renderColumn(column_ - 1, te);
            displayWaitVSync();
            displayUpdate(getRenderBufferChunk(column_, te), te.pixelHeight(), [&]() {
                // move to previous (right to left column), if there is none, we are done rendering
                if (--column_ < 0)
                    return;
                // render the column we currently point to (already in the renderBuffer)
                displayUpdate(getRenderBufferChunk(column_, te), te.pixelHeight());
                // render the column ahead, if any
                if (column_ > 0)
                    renderColumn(column_ - 1, te);
            });
        }
    
    private:

        /** Returns the buffer part to be used for given column as the buffer is twice the column height. 
         */
        ColorRGB * getRenderBufferChunk(Coord column, UITileEngine<TILE> const & te) {
            return renderBuffer_ + (column & 1) * te.pixelHeight();
        }

        /** Renders the given column. 
         */
        void renderColumn(Coord column, UITileEngine<TILE> const & te) {
            ColorRGB * rBuffer = getRenderBufferChunk(column, te);
            // get the tile x coordinate and the x coordinate within the tile
            int tx = column / TILE::Width;
            // get first tile info for the given column (the column is consecutive tiles), and iterate over the tiles
            UITileInfo const * tile = & te.at(tx, 0);
            ColorRGB * rb = rBuffer;
            for (int ty = 0; ty < te.h_; ++ty, ++tile)
                rb = te.tiles_[static_cast<uint8_t>(tile->c)].renderColumn(column % TILE::Width, rb, te.palette(), tile->paletteOffset);
            // render sprites & bitmaps, if any 
            for (Sprite<typename TILE::Color> const * sprite : te.sprites_)
                sprite->renderColumn(column, rBuffer, te.pixelHeight(), te.palette());

        }

        int column_;
        // rendering buffer for 2 columns (double buffering single column)
        ColorRGB * renderBuffer_ = nullptr;

    }; // rckid::Renderer<UITileEngine>

}