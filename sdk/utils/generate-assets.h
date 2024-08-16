#pragma once 

#include <string>
#include <ostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

#include <raylib.h>

#include <platform/string_utils.h>

#include "rckid/graphics/font.h"

class Asset {
public:
    std::string header;
    std::string definition;
}; 

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

std::vector<int> getAlphaNumericGlyphs() {
    std::vector<int> result;
    for (int i = 32; i < 127; ++i)
        result.push_back(i);
    std::cout << "        generating alphanumeric glyphs (" << result.size() << " glyphs)" << std::endl;
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

/** Generates font glyphs for given font. Unlike font tiles, glyphs can have different sizes for different glyphs, supporting both monospaced and proportional fonts. 
*/
inline Asset generateFontGlyphs(GeneratorSpecification const & g) {
    if (g.args.size() != 2)
        throw std::runtime_error("Invalid number or arguments, usage: fontGlyphs, FONT_NAME, FONT_SIZE");
    std::string fontFile{g.args[0]};
    int fontSize = std::atoi(g.args[1].c_str());
    std::string assetName{STR(std::filesystem::path{fontFile}.stem().c_str() << fontSize)};
    std::cout << "    asset font::" << assetName << std::endl;
    std::vector<int> glyphs{getAlphaNumericGlyphs()};
    GlyphInfo * gInfos = loadFontGlyphs(fontFile, fontSize, glyphs);

    std::stringstream hdr;
    std::stringstream def;
    //GlyphInfo * glyphData = loadFontGlyphs(font, fontSie, glyphs);

    return Asset{hdr.str(), def.str()};
}