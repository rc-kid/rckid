#pragma once 

#include <string>
#include <ostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>

#include <raylib.h>

#include <platform/string_utils.h>

#include "rckid/graphics/font.h"

class GeneratorSpecification {
public:
    std::string name;
    std::vector<std::string> args;

    GeneratorSpecification(std::string const & fromLine) {
        std::stringstream ls{fromLine};
        if (! std::getline(ls, name, ','))
            throw std::runtime_error("No asset generator specified");
        trimInPlace(name);
        std::string arg;
        while (std::getline(ls, arg, ','))
            args.push_back(trim(arg));
    }
};


class GlyphIds {
public:
    std::vector<std::string> names;
    std::vector<int> codepoints;
}; 

/** Loads glyph indices from given file. The file is expected to be in the `assets/font-glyphs-inc.h` formet, i.e. 
 
        GLYPH(NAME, CODEPOINT)
        GLYPH(NAME, CODEPOINT)
        ...

    TODO this will only be useful for symbol fonts (!)
 */
GlyphIds loadGlyphIndices(std::string const & fromFile) {
    std::cout << "        loading glyphs from " << fromFile << std::endl;
    std::ifstream gf(fromFile);
    if (!gf.good())
        throw std::runtime_error(STR("Cannot open file " << fromFile));
    GlyphIds result;
    std::string l;
    while (std::getline(gf, l)) {
        std::stringstream sl{l};
        std::string tmp;
        std::getline(sl, tmp);
        result.names.push_back(trim(tmp).substr(6));
        std::getline(sl, tmp);
        result.codepoints.push_back(std::atoi(trim(tmp).c_str()));
    }
    std::cout << "            loaded " << result.names.size() << " glyphs" << std::endl;
    return result;
}

GlyphIds getAlphaNumericGlyphs() {
    GlyphIds result;
    for (int i = 32; i < 127; ++i) {
        result.names.push_back(STR((char)i));
        result.codepoints.push_back(i);
    }
    std::cout << "        generating alphanumeric glyphs (" << result.codepoints.size() << " glyphs)" << std::endl;
    return result;
}


/** Using Raylib, loads the given font and extract given glyphs (UTF codepoints). 
 */
GlyphInfo * loadFontGlyphs(std::string const & fontFile, int fontSize, std::vector<int> const & glyphs) {
    std::cout << "        loading font " << fontFile << std::endl;
    std::ifstream input(fontFile, std::ios::binary);
    std::vector<unsigned char> bytes(
         (std::istreambuf_iterator<char>(input)),
         (std::istreambuf_iterator<char>()));
    input.close();   
    GlyphInfo * glyphInfos = LoadFontData(bytes.data(), bytes.size(), fontSize, const_cast<int*>(glyphs.data()), glyphs.size(), FONT_DEFAULT);
    std::cout << "            loaded " << glyphs.size() << " glyphs" << std::endl;
    return glyphInfos;
}

inline void drawImageComments(Image const & img, std::ostream & s) {
    for (int y = 0; y < img.height; ++y) {
        s << "            // ";
        for (int x = 0; x < img.width; ++x) {
            Color cc = GetImageColor(img, x, y);
            unsigned c = (cc.r + cc.g + cc.b) / 3;
            c = std::min(255u, c + 32); // rounding 
            c >>= 6; // convert tp 2bpp
            switch (c) {
                case 0: 
                    s << "  ";
                    break;
                case 1:
                    s << "\u2591\u2591";
                    break;
                case 2:
                    s << "\u2593\u2593";
                    break;
                case 3:
                    s << "\u2588\u2588";
                    break;
            }
        }
        s << std::endl;
    }
    s << std::endl;
}

inline std::string generateFontGlyphs(std::string const & className, std::string const & fontFile, int fontSize, GlyphIds const & glyphs) {
    std::cout << "    asset font::" << className << std::endl;
    std::stringstream defGlyphs;
    std::stringstream defPixels;
    // load the font
    GlyphInfo * gInfos = loadFontGlyphs(fontFile, fontSize, glyphs.codepoints);
    // prepare the header and contents 
    // take the glyphs and add
    size_t gIndex = 0;
    for (size_t i = 0; i < glyphs.codepoints.size(); ++i) {
        size_t currentIndex = gIndex;
        GlyphInfo const & g = gInfos[i];
        defPixels << "            // " << i << " (" << glyphs.names[i] << ", codepoint: " << glyphs.codepoints[i] << " utf: " << encodeUTF8(glyphs.codepoints[i]) << ", offset " << currentIndex << ")" << std::endl;
        defPixels << "            ";
        // generate the glyph pixels, which we do column wise, starting from left (because this is how we will render the fonts, from left to right)
        for (int x = 0; x < g.image.width; ++x) {
            uint32_t data = 0;
            int bits = 0;
            for (int y = 0; y < g.image.height; ++y) {
                unsigned c = GetImageColor(g.image, x, y).r;
                c = std::min(255u, c + 32); // rounding 
                c >>= 6; // convert tp 4bpp
                data <<= 2;
                data = data | c;
                bits += 2;
                if (bits == 32) {
                    defPixels << data << ",";
                    data = 0;
                    bits = 0;
                    ++gIndex;
                }
            }
            // we require fornt data to align to 32bit boundaries for each row so if the width of the glyph is not proper, we have to padd it now
            if (bits != 0) {
                data <<= (32 - bits);
                defPixels << data << ",";
                ++gIndex;
            }
        }
        defPixels << std::endl;
        size_t glyphHeight = g.image.height;
        if (glyphHeight % 16 != 0)
            glyphHeight += 16 - (glyphHeight % 16);
        defGlyphs << "            GlyphInfo{" << currentIndex << ", " << g.advanceX << ", " << g.offsetX << ", " << g.offsetY << ", " << g.image.width << ", " << glyphHeight << "}, // " << i << " (" << glyphs.names[i] << ", codepoint: " << glyphs.codepoints[i] << " utf: " << encodeUTF8(glyphs.codepoints[i]) << ")" << std::endl; 
        drawImageComments(g.image, defGlyphs);
    }
    
    return STR(
        "    /* Autogenerated font, from file " << fontFile << std::endl << std::endl <<
        "       Number of glyphs : " << glyphs.codepoints.size() << std::endl <<
        "       GlyphInfo size:    " << 8 * glyphs.codepoints.size() << std::endl <<
        "       Pixels size:       " << gIndex * 4 << std::endl << 
        "       Total size:        " << (8 * glyphs.codepoints.size() + 4 * gIndex) << " bytes" << std::endl <<
        "     */" << std::endl <<
        "    class " << className << " {" << std::endl <<
        "    public:" << std::endl <<
        "        static constexpr int size = " << fontSize << ";" << std::endl <<
        "        static constexpr GlyphInfo glyphs[] = {" << std::endl <<
        defGlyphs.str() <<
        "        }; // " << className << "::glyphs" << std::endl << std::endl <<
        "        static constexpr uint32_t pixels[] = {" << std::endl <<
        defPixels.str() <<
        "        }; // " << className << "::pixels" << std::endl <<
        "    }; // " << className << std::endl
    );
}

/** Generates font glyphs for given font. Unlike font tiles, glyphs can have different sizes for different glyphs, supporting both monospaced and proportional fonts. 
*/
inline void generateFontGlyphs(GeneratorSpecification const & g, std::string const & outputDir, std::string const & nspace) {
    if (g.args.size() != 2)
        throw std::runtime_error("Invalid number or arguments, usage: fontGlyphs, FONT_NAME, FONT_SIZE");
    std::string fontFile{g.args[0]};
    int fontSize = std::atoi(g.args[1].c_str());
    std::string className{STR(std::filesystem::path{fontFile}.stem().c_str() << fontSize)};
    GlyphIds glyphs{getAlphaNumericGlyphs()};
    std::filesystem::create_directories(outputDir + "/fonts");
    std::ofstream def(outputDir + "/fonts/" + className + ".h");
    def << "#pragma once" << std::endl << std::endl;
    def << "// DO NOT EDIT THIS FILE, IT HAS BEEN AUTOGENERATED BY generate-assets" << std::endl << std::endl;
    def << "#include <rckid/graphics/font.h>" << std::endl << std::endl;
    def << "namespace " << nspace << "::font {" << std::endl << std::endl;
    def << generateFontGlyphs(className, fontFile, fontSize, glyphs);
    def << "} // namesapce " << nspace << "::font" << std::endl;
}