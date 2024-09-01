#include <cstdlib>
#include <cstdio>



#include "generate-assets.h"

// TODO maybe move this to assets hdr file when generating tiles from bitmaps? 
class Tile {
public:
    int width;
    int height;
    std::vector<int> data;
    bool clipped = false;

    Tile(int w, int h):
        width{w},
        height{h},
        data(w * h) {
    }

    void drawFontGlyph(GlyphInfo & g) {
        int sx = (width - g.image.width) / 2;
        int sy = (height - g.image.height) / 2;
        for (int x = 0; x < g.image.width; ++x) {
            for (int y = 0; y < g.image.height; ++y) {
                unsigned c = GetImageColor(g.image, x, y).r;
                c = std::min(255u, c + 8); // rounding 
                c >>= 4; // convert tp 4bpp (16 colors)
                setPixel(sx + x, sy + y, c);
            }
        }
    }

    void setPixel(int x, int y, int value) {
        if (x < 0 || y < 0 || x >= width || y >= height) {
            clipped = true;
            return;
        }
        data[ x + y * width] = value;
    }

    void printBytes(std::ostream & s, std::string prefix) {
        for (int y = 0; y < height; ++y) {
            s << prefix;
            for (int x = 0; x < width; ++x) {
                int value = data[x + y * width];
                if (value == 0)
                    s << "_, ";
                else 
                    s << value << ((value < 10) ? ", " : ",");
            }
            s << std::endl;
        }
    }
}; 

/** Creates font tiles. 
 
    Usage:

        create-font-tiles PATH_TO_FONT FONT_SIZE TILE_WIDTH TILE_HEIGHT PATH_TO_CODEPOINT_DEFS OUTPUT FILE

    Where:

        PATH_TO_FONT is path to the font from which the glyphs should be taken
        FONT_SIZE is the size of the font used for the glyphs (can be different than tilegeight, glyphs will be centered and/or clipped)
        TILE_WIDTH is width of the tile
        TILE_HEIGHT is height of the tile. All tiles are Color16 and therefore only multiples of 8 are proper values here.
        PATH_TO_CODEPOINT_DEFS is path to codepoint definitions header file (same as for symbol glyphs in asset generator \ref assets)
        OUTPUT_FILE is where the definitions will be put (the containing folder must exist)

    The font tiles are likely to be manually updated and edited afterwords an so this is not part of the automated asset generation. 

    build/sdk/utils/create-font-tiles sdk/assets/fonts/Iosevka.ttf 24 12 24 sdk/rckid/assets/ui-tile-glyphs.inc.h sdk/rckid/assets/tiles/ui-tiles.h

 */
int main(int argc, char * argv[]) {
    try {
        if (argc != 7)
            throw std::runtime_error("Invalid number of arguments");
        std::string fontFile = argv[1];
        int fontSize = std::atoi(argv[2]);
        int tileWidth = std::atoi(argv[3]);
        int tileHeight = std::atoi(argv[4]);
        std::string codepointsFile = argv[5];
        std::string outputFile = argv[6];
        // load the glyphs
        GlyphIds glyphs{loadGlyphIndices(codepointsFile)};
        // load the font
        GlyphInfo * gInfos = loadFontGlyphs(fontFile, fontSize, glyphs.codepoints);
        std::vector<Tile> tiles;
        size_t numClipped = 0;
        for (size_t i = 0; i < glyphs.codepoints.size(); ++i) {
            Tile t(tileWidth, tileHeight);
            t.drawFontGlyph(gInfos[i]);
            if (t.clipped)
                ++numClipped;
            tiles.push_back(t);
        }
        std::ofstream out(outputFile);
        out << "#pragma once" << std::endl << std::endl;
        out << "#include \"rckid/graphics/tile.h\"" << std::endl << std::endl;
        out << "namespace rckid {" << std::endl << std::endl;
        out << "    /** Tiles " << std::endl << std::endl;
        out << "        The class contents was initially generated from font glyphs using create-font-tiles: " << std::endl << std::endl;
        out << "            create-font-tiles " << fontFile << " " << fontSize << " " << tileWidth << " " << tileHeight << " " << codepointsFile << " " << outputFile << std::endl << std::endl;
        out << "        Codepoints:  " << glyphs.codepoints.size() << std::endl;
        out << "        Clipped:     " << numClipped << std::endl;
        out << "        Font size:   " << fontSize << std::endl;
        out << "        Tile width:  " << tileWidth << std::endl;
        out << "        Tile height: " << tileHeight << std::endl;
        out << "        Total bytes: " << (glyphs.codepoints.size() * tileWidth * tileHeight / 2) << std::endl;
        out << "     */" << std::endl;
        out << "    class Tiles {" << std::endl;
        out << "    public:" << std::endl;
        out << "        static constexpr uint32_t NumTiles = " << glyphs.codepoints.size() << ";" << std::endl;
        out << "        static constexpr Coord TileWidth = " << tileWidth << ";" << std::endl;
        out << "        static constexpr Coord TileHeight = " << tileHeight << ";" << std::endl << std::endl;
        out << "        using Color = Color16;" << std::endl;
        out << "        using Tile = rckid::Tile<" << tileWidth << ", " << tileHeight << ", Color16>;" << std::endl << std::endl;
        out << "        static constexpr uint8_t _ = 0;" << std::endl;
        out << "        static constexpr Tile Tileset[] = {" << std::endl;
        for (size_t i = 0; i < glyphs.codepoints.size(); ++i) {
            out << "            Tile({ // " << i << " (" << glyphs.names[i] << ", codepoint: " << glyphs.codepoints[i] << " utf: " << encodeUTF8(glyphs.codepoints[i]) << (tiles[i].clipped ? ", clipped)" : ")") << std::endl;
            tiles[i].printBytes(out, "                ");
            out << "            })," << std::endl;
        }
        out << "        }; // Tileset" << std::endl << std::endl;
        out << "    }; // class Tiles" << std::endl;
        out << "} // namespace rckid" << std::endl;
        return EXIT_SUCCESS;
    } catch (std::exception const & e) {
        std::cout << "ERROR: " << e.what() << std::endl << std::endl;
        return EXIT_FAILURE;
    }




}