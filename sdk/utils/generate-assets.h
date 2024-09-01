#pragma once 

#define _USE_MATH_DEFINES
#include <cmath>
#include <string>
#include <ostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <filesystem>

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
        std::getline(sl, tmp, ',');
        trimInPlace(tmp);
        if (tmp.empty())
            continue;
        result.names.push_back(trim(tmp).substr(6));
        std::getline(sl, tmp);
        // we need the 0x notation whih std::atoi won't give us
        char * end;
        result.codepoints.push_back(std::strtol(trim(tmp).c_str(), & end, 0));
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
    GlyphInfo * glyphInfos = LoadFontData(bytes.data(), (int) bytes.size(), fontSize, const_cast<int*>(glyphs.data()), (int) glyphs.size(), FONT_DEFAULT);
    std::cout << "            loaded " << glyphs.size() << " glyphs" << std::endl;
    return glyphInfos;
}

/** Draws the contents of the given image in 4 layers of gray (shaded boxes) as C++ comments to be used in the aset header files for visual inspection.
 */
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

/** Generates font glyphs as assets. 
 */
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

/** Reads given file and outputs it as a contents of C++ array of uint8_t (without the declaration) to the given output stream. A helper function intended to be used by other generators. Returns the size in bytes of the input file (which is how much ROM the asset will take)
 */
inline size_t generateBinaryContent(std::filesystem::path const & inputFile, std::ostream & out) {
    std::ifstream input(inputFile, std::ios::binary);
    std::vector<char> bytes(
         (std::istreambuf_iterator<char>(input)),
         (std::istreambuf_iterator<char>()));
    input.close();   
    out << "// DO NOT EDIT THIS FILE, IT HAS BEEN AUTOGENERATED BY generate-assets" << std::endl;
    out << "// Input file: " << inputFile << std::endl;
    out << "// Size:       " << bytes.size() << " bytes" << std::endl;
    size_t i = 0; 
    for (char c : bytes) {
        unsigned x = *reinterpret_cast<unsigned char*>(& c);
        out << x << ",";
        if (++i % 16 == 0)
            out << std::endl;
    }
    return bytes.size();
}

/** Converts given string to a valid C++ class name (or identifier). Ignores leading numbers and hyphens and converts hyphens to underscored. Note that his is very light mangling and as such does not guarantee that what wee unique inputs to the function will be unique results as well, it mostly exists as a convenience fuction to quickly converts folder contents. 
 */
inline std::string convertToClassName(std::filesystem::path const & from) {
    std::string result;
    bool start = true;
    for (char c : from.string()) {
        if (start && c >= '0' && c <= '9')
            continue;
        if (start && c == '-')
            continue;
        if (start && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
            start = false;
        if (c == '-')
            c = '_';
        result += c;
    }
    return result;
}


/** Generates font glyphs for given font. Unlike font tiles, glyphs can have different sizes for different glyphs, supporting both monospaced and proportional fonts. 
*/
inline void generateFontGlyphs(GeneratorSpecification const & g, std::string const & outputDir, std::string const & nspace) {
    
    if (g.args.size() < 2 || g.args.size() > 4)
        throw std::runtime_error("Invalid number or arguments, usage: fontGlyphs, FONT_NAME, FONT_SIZE [CLASS_NAME [ GLYPHS_DEF]]");
    std::string fontFile{g.args[0]};
    int fontSize = std::atoi(g.args[1].c_str());
    std::string className{STR(std::filesystem::path{fontFile}.stem().c_str() << fontSize)};
    if (g.args.size() >= 3)
        className = g.args[2];
    GlyphIds glyphs{(g.args.size() == 4) ? loadGlyphIndices(g.args[3]) : getAlphaNumericGlyphs()};
    std::filesystem::create_directories(outputDir + "/fonts");
    std::ofstream def(outputDir + "/fonts/" + className + ".h");
    def << "#pragma once" << std::endl << std::endl;
    def << "// DO NOT EDIT THIS FILE, IT HAS BEEN AUTOGENERATED BY generate-assets" << std::endl << std::endl;
    def << "#include <rckid/graphics/font.h>" << std::endl << std::endl;
    def << "namespace " << nspace << "::font {" << std::endl << std::endl;
    def << generateFontGlyphs(className, fontFile, fontSize, glyphs);
    def << "} // namesapce " << nspace << "::font" << std::endl;
}

/** Converts all files in the specified folder into an asset namespace. Each file will be one static constexpr array in the namespace. To ease the editing the actual raw contents of the files is stored in a header file per input file and the header files are included into the definitions. 
 */
void generateFolder(GeneratorSpecification const & g, std::string const & outputDir, std::string const & nspace) {
    if (g.args.size() != 2)
        throw std::runtime_error("Invalid number or arguments, usage: folder, PATH, namespace");
    namespace fs = std::filesystem;
    std::string folder = g.args[0];
    std::string outName = g.args[1];
    fs::create_directories(STR(outputDir << "/raw/" << outName));
    std::ofstream hdr(STR(outputDir << "/" << outName << ".h"));
    hdr << "#pragma once" << std::endl << std::endl;
    hdr << "// DO NOT EDIT THIS FILE, IT HAS BEEN AUTOGENERATED BY generate-assets" << std::endl << std::endl;
    hdr << "#include \"../rckid.h\"" << std::endl << std::endl;
    hdr << "// Folder: " << folder << std::endl;
    hdr << "namespace " << nspace << "::" << outName << "{" << std::endl;
    for (auto const & entry : fs::directory_iterator{folder}) {
        std::string name = convertToClassName(entry.path().stem());
        std::string outputFile = STR(outputDir << "/raw/" << outName << "/" << entry.path().filename().c_str() << ".h");
        std::ofstream outputStream(outputFile);
        size_t numBytes = generateBinaryContent(entry.path(), outputStream);
        hdr << std::endl;
        hdr << "    // File: " << entry.path().filename().c_str() << std::endl;
        hdr << "    // Size: " << numBytes << std::endl;
        hdr << "    static constexpr uint8_t " << name << "[] = {" << std::endl;
        hdr << "        #include \"raw/" << outName << "/" << entry.path().filename().c_str() << ".h\"" << std::endl;
        hdr << "    };" << std::endl;
    }
    hdr << "} // namespace " << nspace << "::" << outName << std::endl;
}

/** Very simple generator of sine tables for quick interolation. 
 
    Note that the generator only creates the sine table contents itself and must be linked with the appropriate array (we do not expect to have very many of these)
 */
void generateSineTable(GeneratorSpecification const & g, std::string const & outputDir, [[maybe_unused]] std::string const & nspace) {
    if (g.args.size() != 2)
        throw std::runtime_error("Invalid number or arguments, usage: sine, FILENAME, N");
    std::string filename{g.args[0]};
    unsigned n = std::atoi(g.args[1].c_str());
    std::ofstream f(STR(outputDir << "/" << filename));
    f << "// DO NOT EDIT THIS FILE, IT HAS BEEN AUTOGENERATED BY generate-assets" << std::endl;
    f << "// sine table (length << " << n << ")" << std::endl;
    for (unsigned i = 0; i < n; ++i) {
        if (i % 8 == 0)
            f << std::endl;
        // ensure the last value is max
        if (i == n - 1)
            f << "65535, ";
        else 
            f << static_cast<unsigned>(std::sin(i / (n * 2.0) * M_PI) * 65535) << ", ";  
    }
}

