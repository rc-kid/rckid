#pragma once

namespace rckid {


    template<unsigned BPP>
    class Tile {
    public:
        static constexpr unsigned BPP = BPP;

    private:

        uint32_t data_[16 * 16 * BPP / 32];

    }; // rckid::Tile

    template<typename TILE>
    class TileMap {
    public:
        static constexpr unsigned BPP = TILE::BPP;

    private:
        TILE const * tileSet_;
        int width_;
        int height_;
        int top_;
        int left_;
        uint8_t * tileMap_;
    }; 

    template<typename TILE>
    class TileEngine {
    public:
        static constexpr unsigned BPP = TILE::BPP;

        void enable() {

        }

        void disable() {

        }

        void render() {

        }

    private:

        /** Renders the given column into provided buffer. 
         
            Takes layer0, 
        */
        void renderColumn(int col) {

        }
        

    };
}