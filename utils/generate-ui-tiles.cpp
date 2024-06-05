#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <raylib.h>

#include "helpers.h"

static constexpr int WIDTH = 12;
static constexpr int HEIGHT = 24;
static constexpr int FONTSIZE = 24;

// list of glyphs that will be part of the font
int GLYPHS[] = {
    0x2588,
    0xe0b8,
    0xf0b08,

    0xf004, 0xf08a, //  
    0xf05a9, 0xf05aa, 0xf16c1, // 󰖩 󰖪 󱛁
    0xf244, 0xf243, 0xf242, 0xf241, 0xf240, 0xf0e7, //      
    0xf02cb, // 󰋋
    0xf1119, // 󱄙
    0xf0e08, 0xf057f, 0xf0580, 0xf057e, // 󰸈 󰕿 󰖀 󰕾
    0xf04d, 0xf04c, 0xf04b, 0xf01e, //     
    0xf0e7a, 0xf0e74, 0xf047, // 󰹺󰹴 
    0xf07dc, 0xf287, // 󰟜 

    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, // space & various punctuations
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, // 0..9
    58, 59, 60, 61, 62, 63, 64, // more punctuations
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, // A-Z
    91, 92, 93, 94, 95, 96, // more punctuations
    97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, // a-z
    123, 124, 125, 126, // more punctiations
};

int SYMBOLS[] = {
    0xf004, 0xf08a, //  
    0xf05a9, 0xf05aa, 0xf16c1, // 󰖩 󰖪 󱛁
    0xf244, 0xf243, 0xf242, 0xf241, 0xf240, 0xf0e7, //      
    0xf02cb, // 󰋋
    0xf1119, // 󱄙
    0xf0e08, 0xf057f, 0xf0580, 0xf057e, // 󰸈 󰕿 󰖀 󰕾
    0xf04d, 0xf04c, 0xf04b, 0xf01e, //     
    0xf0e7a, 0xf0e74, 0xf047, // 󰹺󰹴 
    0xf07dc, 0xf287 // 󰟜 
}; 

struct Glyph {
    int advanceX;
    int x;
    int y;
    int width;
    int height;
    int codepoint;
    std::string utf8;
    // actual pixel data
    uint8_t * pixels = new uint8_t[WIDTH * HEIGHT];
    bool clipped = false;

    Glyph(int advanceX, int x, int y, int width, int height, int codepoint, std::string const & utf8): advanceX{advanceX}, x{x}, y{y}, width{width}, height{height}, codepoint{codepoint}, utf8{utf8} {
        for(int i = 0; i < WIDTH * HEIGHT; ++i)
            pixels[i] = 0;
    }

    void setPixel(int x, int y, uint8_t value) { 
        if (x < 0 || x >= WIDTH) {
            clipped = true;
            return;
        }
        if (y < 0 || y >= HEIGHT) {
            clipped = true;
            return;
        }
        pixels[y + (x) * HEIGHT] = value;
    }

    uint8_t getPixel(int x, int y) const {
        return pixels[y + (x) * HEIGHT];
    }

    uint32_t toBpp16(int i) const {
        if (pixels[i] > (255 - 8))
            return 15;
        return static_cast<uint32_t>(pixels[i] + 8) >> 4;
    }
};

/** Generates font tiles from local fonts to be used on the RCKid. 
 
    Usage

        generate-font-tiles SOURCE_FONT TARGET_DIRECTORY

    The tiles are 16x16 pixels width 
 */
int main(int argc, char * argv[]) {
    int * glyphsToGenerate = GLYPHS;
    int numGlyphs = sizeof (GLYPHS) / sizeof(int);
    std::ifstream input(argv[1], std::ios::binary);
    std::vector<unsigned char> bytes(
         (std::istreambuf_iterator<char>(input)),
         (std::istreambuf_iterator<char>()));
    input.close();   
    GlyphInfo * glyphInfos = LoadFontData(bytes.data(), bytes.size(), FONTSIZE, glyphsToGenerate, numGlyphs, FONT_DEFAULT);

    std::cout << "Loaded " << numGlyphs << " glyphs" << std::endl;
    std::cout << "Calculating glyph data..." << std::endl;
    std::vector<Glyph> glyphs;
    size_t clipped = 0;
    for (int i = 0; i < numGlyphs; ++i) {
        GlyphInfo gi = glyphInfos[i];
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
        std::string utf8 = converter.to_bytes(glyphsToGenerate[i]);
        Glyph g{
            gi.advanceX, 
            gi.offsetX, 
            gi.offsetY,
            gi.image.width, 
            gi.image.height,
            glyphsToGenerate[i],
            utf8
        };
        // get the offsets, based on width and height of the glyph and the tile
        int ox = (WIDTH - g.width) / 2;
        //std::cout << g.advanceX << " -- " << ox << std::endl;
        int oy = (HEIGHT - FONTSIZE) / 2;
        // calculate the pixels
        for (int x = 0; x < g.width; ++x) {
            for (int y = 0; y < g.height; ++y) {
                // get the value rounded to bpps
                uint8_t c = GetImageColor(gi.image, x, y).r;
                g.setPixel(g.x + ox + x, g.y + oy + y, c);
            }
        }
        if (g.clipped)
            ++clipped;
        glyphs.push_back(g);
    }
    std::cout << "Glyphs array byte size: " << glyphs.size() * WIDTH * HEIGHT / 2 << std::endl;
    std::cout << "Writing to output file... " << std::endl;
    std::string className = STR(baseNameOf(fs::path(argv[1])) << "_Tiles_" << WIDTH << "x" << HEIGHT);
    std::ofstream hdr{std::string(argv[2]) + "/" + className + ".h"};
    hdr << "#pragma once" << std::endl << std::endl;
    hdr << "/* This is autogenerated tile definition file. Do not edit manually." << std::endl;
    hdr << "   font file:      " << argv[1] << std::endl;
    hdr << "   font size:      " << FONTSIZE << std::endl;
    hdr << "   tile width:     " << WIDTH << std::endl;
    hdr << "   tile height:    " << HEIGHT << std::endl;
    hdr << "   tiles:          " << glyphs.size() << std::endl;
    hdr << "   clipped:        " << clipped << std::endl;
    hdr << "   bytes:          " << (glyphs.size() * WIDTH * HEIGHT / 2) << std::endl;
    hdr << " */" << std::endl << std::endl;
    hdr << "#define TILE(\\" << std::endl;
    // TODO the inputs
    for (int i = 0; i < HEIGHT; ++i) {
        hdr << "    ";
        for (int j = 0; j < WIDTH; ++j)
            hdr << "X_" << i << "_" << j << ", ";
        hdr << "\\" << std::endl;
    }
    hdr << ") \\" << std::endl;
    // TODO define the permutations
    hdr << std::endl << std::endl;
    hdr << "namespace rckid {" << std::endl;
    hdr << "    class " << className << " {" << std::endl;
    hdr << "    public:" << std::endl << std::endl;
    hdr << "        static constexpr int width = " << WIDTH << ";" << std::endl;
    hdr << "        static constexpr int height = " << HEIGHT << ";" << std::endl;
    hdr << "        static constexpr int tiles = " << glyphs.size() << ";" << std::endl << std::endl;
    hdr << "        static constexpr uint32_t tileset[] = {" << std::endl;
    {
        int i = 0;
        for (auto const & gi : glyphs) {
            // convert the alpha channel to 16 colors and print out the results, 8 pixels per uint32_t
            hdr << "            ";
            for (int i = 0; i < WIDTH * HEIGHT; i += 8) {
                uint32_t x = 0;
                unsigned shift = 0;
                for (int j = 0; j < 8; ++j) {
                    x |= (gi.toBpp16(i + j) << shift);
                    shift += 4;
                }
                hdr << x << ", ";
            } 
            // write the comments
            hdr << "// " << i << " (codepoint: " << gi.codepoint << ", '" << gi.utf8 << (gi.clipped ? "', clipped" : "'") <<  ")" << std::endl; 
            for (int y = 0; y < HEIGHT; ++y) {
                hdr << "            // ";
                for (int x = 0; x < WIDTH; ++x) {
                    uint8_t c = gi.getPixel(x, y);
                    if (c < 64)
                        hdr << "  ";
                    else if (c < 128)
                        hdr << "\u2591\u2591";
                    else if (c < 196)
                        hdr << "\u2592\u2592";
                    else 
                        hdr << "\u2588\u2588";
                }
                hdr << std::endl;
            }
            hdr << std::endl;
            ++i;
        }
    }
    hdr << "        }; // tileset" << std::endl;
    hdr << "    }; // " << className << std::endl;
    hdr << "} // namespace rckid" << std::endl;

    UnloadFontData(glyphInfos, numGlyphs);

    return EXIT_SUCCESS;
}