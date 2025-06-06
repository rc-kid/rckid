#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <raylib.h>
#include <platform.h>
#include <platform/string_utils.h>

inline void generateAssetsFileHeader(int argc, char const * argv[], std::ostream & s) {
    s << "#pragma once" << std::endl;
    s << "/* DO NOT EDIT THIS FILE, IT HAS BEEN AUTOGENERATED BY THE FOLLOWING COMMAND: " << std::endl;
    s << "   ";
    s << std::filesystem::path{argv[0]}.stem().string() << " ";
    for (int i = 1; i < argc; ++i)
        s << " " << argv[i];
    s << std::endl;
    s << " */" << std::endl << std::endl;
}

/** Converts given string to a valid C++ class name (or identifier). Ignores leading numbers and hyphens and converts hyphens to underscored. Note that his is very light mangling and as such does not guarantee that what wee unique inputs to the function will be unique results as well, it mostly exists as a convenience fuction to quickly converts folder contents. 
 */
inline std::string convertToClassName(std::string const & from) {
    std::string result;
    bool start = true;
    for (char c : from) {
        if (start && c >= '0' && c <= '9')
            continue;
        if (start && c == '-')
            continue;
        if (start && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
            start = false;
        if (c == '-' || c == ' ' || c == ',' || c == '(' || c == ')')
            c = '_';
        result += c;
    }
    return result;
}

struct Glyphs {
    std::vector<std::string> names;
    std::vector<int> codepoints;

    size_t size() const { return codepoints.size(); }
}; // Glyphs


/** Loads glyphs from given file. The file is expected to be in the font glyphs format that is also used by the C++ code, i.e. c++ include file with calls to GLYPH macro for each used glyph. The macro takes two arguments - glyph name and a codepoint (utf). The glyphs parser is permissive, any lines that do not start with GLYPH are ignored.

        GLYPH(NAME, CODEPOINT)
        GLYPH(NAME, CODEPOINT)
        ...
 */
inline Glyphs loadGlyphIndices(std::string const & fromFile) {
    std::cout << "        loading glyphs from " << fromFile << std::endl;
    std::ifstream gf(fromFile);
    if (!gf.good())
        throw std::runtime_error(STR("Cannot open file " << fromFile));
    Glyphs result;
    std::string l;
    while (std::getline(gf, l)) {
        // undefs and comments are ignored - note this is very much not sturdy
        if (!startsWith(l.c_str(), "GLYPH"))
            continue;
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

/** Generates the default alphanumeric glyphs used for fonts, which correspond to printable ASCII characters. Those glyphs are used if no glyph definitions are explicitly provided.
 */
inline Glyphs getDefaultGlyphs() {
    Glyphs result;
    for (int i = 32; i < 127; ++i) {
        result.names.push_back(STR((char)i));
        result.codepoints.push_back(i);
    }
    std::cout << "        generating alphanumeric glyphs (" << result.codepoints.size() << " glyphs)" << std::endl;
    return result;
}

/** Default font glyphs for tiles, where the index of the tile corresponds to the ASCII character. 
 */
inline Glyphs getDefaultTileGlyphs () {
    Glyphs result;
    for (int i = 0; i < 32; ++i) {
        result.names.push_back(" ");
        result.codepoints.push_back(32);
    }
    for (int i = 32; i < 127; ++i) {
        result.names.push_back(STR((char)i));
        result.codepoints.push_back(i);
    }
    std::cout << "        generating tile glyphs (" << result.codepoints.size() << " glyphs)" << std::endl;
    return result;
}

/** Takes the specified font, font size and glyphs and uses Raylib to generate character glyphs from the font, returning their array. 
 */
inline GlyphInfo * loadFontGlyphs(std::string const & fontFile, int fontSize, Glyphs const & glyphs) {
    std::cout << "        loading font " << fontFile << std::endl;
    std::ifstream input(fontFile, std::ios::binary);
    std::vector<unsigned char> bytes(
         (std::istreambuf_iterator<char>(input)),
         (std::istreambuf_iterator<char>()));
    input.close();   
    GlyphInfo * glyphInfos = LoadFontData(bytes.data(), (int) bytes.size(), fontSize, const_cast<int*>(glyphs.codepoints.data()), (int) glyphs.size(), FONT_DEFAULT);
    std::cout << "            loaded " << glyphs.size() << " glyphs" << std::endl;
    return glyphInfos;
}

/** Generates grayscale image data with given bits per pixel depth and outputs the raw data. 
 */
inline size_t outputImageAsFontGlyph(Image const & img, std::ostream & s) {
    uint8_t data = 0;
    unsigned bits = 0;
    size_t numBytes = 0;
    auto print = [&]() {
        s << (unsigned)data << ",";
        data = 0;
        bits = 0;
        ++numBytes;
    };
    for (int x = 0; x < img.width; ++x) {
        for (int y = 0; y < img.height; ++y) {
            Color cc = GetImageColor(img, x, y);
            unsigned c = (cc.r + cc.g + cc.b) / 3;
            c = std::min(255u, c + 32); // rounding 
            c >>= 6; // convert tp 2bpp
            data = (data << 2) | c;
            bits += 2;
            if (bits == 8)
                print();
        }
        if (bits != 0) {
            data <<= (8 - bits);
            print();
        }
    }
    return numBytes;
}

/** Draws the contents of the given image in 4 layers of gray (shaded boxes) as C++ comments. Takes an image to draw, output stream and indentation as arguments. 
 */
inline void drawImageInComments(Image const & img, std::ostream & s, std::string const & indent) {
    for (int y = 0; y < img.height; ++y) {
        s << indent << "// ";
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

struct FolderItem {
    std::string filename;
    std::string className;
    std::vector<uint8_t> bytes;

    FolderItem(std::filesystem::path const & path):
        className{convertToClassName(path.stem().string())},
        filename{path.string()} {
        std::ifstream input(filename, std::ios::binary | std::ios::ate);
        std::streamsize fileSize = input.tellg();
        input.seekg(0, std::ios::beg);
        bytes.resize(fileSize);
        if (!input.read(reinterpret_cast<char*>(bytes.data()), fileSize))
            throw std::runtime_error(STR("Error reading file " << filename));
        input.close();
    }

    void outputBytes(std::ostream & s, std::string const & indent) const {
        size_t i = 0;
        for (uint8_t c : bytes) {
            if (i % 16 == 0) {
                if (i != 0)
                    s << std::endl;
                s << indent;
            }
            s << (unsigned) c << ",";
            ++i;
        }
    } 
}; // FolderItem
