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
#include "rckid/audio/opus.h"

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
    std::cout << "        loading glyphs from " << fromFile << Writer::endl;
    std::ifstream gf(fromFile);
    if (!gf.good())
        throw std::runtime_error(STR("Cannot open file " << fromFile));
    GlyphIds result;
    std::string l;
    while (std::getline(gf, l)) {
        // undefs and comments are ignored - note this is very much not sturdy
        if (l[0] != 'G')
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
    std::cout << "            loaded " << result.names.size() << " glyphs" << Writer::endl;
    return result;
}

GlyphIds getAlphaNumericGlyphs() {
    GlyphIds result;
    for (int i = 32; i < 127; ++i) {
        result.names.push_back(STR((char)i));
        result.codepoints.push_back(i);
    }
    std::cout << "        generating alphanumeric glyphs (" << result.codepoints.size() << " glyphs)" << Writer::endl;
    return result;
}


/** Using Raylib, loads the given font and extract given glyphs (UTF codepoints). 
 */
GlyphInfo * loadFontGlyphs(std::string const & fontFile, int fontSize, std::vector<int> const & glyphs) {
    std::cout << "        loading font " << fontFile << Writer::endl;
    std::ifstream input(fontFile, std::ios::binary);
    std::vector<unsigned char> bytes(
         (std::istreambuf_iterator<char>(input)),
         (std::istreambuf_iterator<char>()));
    input.close();   
    GlyphInfo * glyphInfos = LoadFontData(bytes.data(), (int) bytes.size(), fontSize, const_cast<int*>(glyphs.data()), (int) glyphs.size(), FONT_DEFAULT);
    std::cout << "            loaded " << glyphs.size() << " glyphs" << Writer::endl;
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
        s << Writer::endl;
    }
    s << Writer::endl;
}

/** Generates font glyphs as assets. 
 */
inline std::string generateFontGlyphs(std::string const & className, std::string const & fontFile, int fontSize, GlyphIds const & glyphs) {
    std::cout << "    asset font::" << className << Writer::endl;
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
        defPixels << "            // " << i << " (" << glyphs.names[i] << ", codepoint: " << glyphs.codepoints[i] << " utf: " << encodeUTF8(glyphs.codepoints[i]) << ", offset " << currentIndex << ")" << Writer::endl;
        defPixels << "            ";
        // generate the glyph pixels, which we do column wise, starting from left (because this is how we will render the fonts, from left to right)
        for (int x = 0; x < g.image.width; ++x) {
            uint32_t data = 0;
            int bits = 0;
            for (int y = 0; y < g.image.height; ++y) {
                unsigned c = GetImageColor(g.image, x, y).r;
                c = std::min(255u, c + 32); // rounding 
                c >>= 6; // convert tp 2bpp
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
        defPixels << Writer::endl;
        size_t glyphHeight = g.image.height;
        if (glyphHeight % 16 != 0)
            glyphHeight += 16 - (glyphHeight % 16);
        defGlyphs << "            GlyphInfo{" << currentIndex << ", " << g.advanceX << ", " << g.offsetX << ", " << g.offsetY << ", " << g.image.width << ", " << glyphHeight << "}, // " << i << " (" << glyphs.names[i] << ", codepoint: " << glyphs.codepoints[i] << " utf: " << encodeUTF8(glyphs.codepoints[i]) << ")" << Writer::endl; 
        drawImageComments(g.image, defGlyphs);
    }
    
    return STR(
        "    /* Autogenerated font, from file " << fontFile << Writer::endl << Writer::endl <<
        "       Number of glyphs : " << glyphs.codepoints.size() << Writer::endl <<
        "       GlyphInfo size:    " << 8 * glyphs.codepoints.size() << Writer::endl <<
        "       Pixels size:       " << gIndex * 4 << Writer::endl << 
        "       Total size:        " << (8 * glyphs.codepoints.size() + 4 * gIndex) << " bytes" << Writer::endl <<
        "     */" << Writer::endl <<
        "    class " << className << " {" << Writer::endl <<
        "    public:" << Writer::endl <<
        "        static constexpr int size = " << fontSize << ";" << Writer::endl <<
        "        static constexpr GlyphInfo glyphs[] = {" << Writer::endl <<
        defGlyphs.str() <<
        "        }; // " << className << "::glyphs" << Writer::endl << Writer::endl <<
        "        static constexpr uint32_t pixels[] = {" << Writer::endl <<
        defPixels.str() <<
        "        }; // " << className << "::pixels" << Writer::endl << Writer::endl <<
        "        static constexpr Font font{size, sizeof(glyphs) / sizeof(GlyphInfo), glyphs, pixels};" << Writer::endl << Writer::endl <<
        "    }; // " << className << Writer::endl
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
    out << "// DO NOT EDIT THIS FILE, IT HAS BEEN AUTOGENERATED BY generate-assets" << Writer::endl;
    out << "// Input file: " << inputFile << Writer::endl;
    out << "// Size:       " << bytes.size() << " bytes" << Writer::endl;
    size_t i = 0; 
    for (char c : bytes) {
        unsigned x = *reinterpret_cast<unsigned char*>(& c);
        out << x << ",";
        if (++i % 16 == 0)
            out << Writer::endl;
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
    def << "#pragma once" << Writer::endl << Writer::endl;
    def << "// DO NOT EDIT THIS FILE, IT HAS BEEN AUTOGENERATED BY generate-assets" << Writer::endl << Writer::endl;
    def << "#include <rckid/graphics/font.h>" << Writer::endl << Writer::endl;
    def << "namespace " << nspace << "::font {" << Writer::endl << Writer::endl;
    def << generateFontGlyphs(className, fontFile, fontSize, glyphs);
    def << "} // namesapce " << nspace << "::font" << Writer::endl;
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
    hdr << "#pragma once" << Writer::endl << Writer::endl;
    hdr << "// DO NOT EDIT THIS FILE, IT HAS BEEN AUTOGENERATED BY generate-assets" << Writer::endl << Writer::endl;
    hdr << "#include \"../rckid.h\"" << Writer::endl << Writer::endl;
    hdr << "// Folder: " << folder << Writer::endl;
    hdr << "namespace " << nspace << "::" << outName << "{" << Writer::endl;
    for (auto const & entry : fs::directory_iterator{folder}) {
        std::string name = convertToClassName(entry.path().stem());
        std::string outputFile = STR(outputDir << "/raw/" << outName << "/" << entry.path().filename().c_str() << ".h");
        std::ofstream outputStream(outputFile);
        size_t numBytes = generateBinaryContent(entry.path(), outputStream);
        hdr << Writer::endl;
        hdr << "    // File: " << entry.path().filename().c_str() << Writer::endl;
        hdr << "    // Size: " << numBytes << Writer::endl;
        hdr << "    static constexpr uint8_t " << name << "[] = {" << Writer::endl;
        hdr << "        #include \"raw/" << outName << "/" << entry.path().filename().c_str() << ".h\"" << Writer::endl;
        hdr << "    };" << Writer::endl;
    }
    hdr << "} // namespace " << nspace << "::" << outName << Writer::endl;
}

/** Very simple generator of sine tables for quick interolation. 
 
    Note that the generator only creates the sine table contents itself and must be linked with the appropriate array (we do not expect to have very many of these)
 */
void generateSineTable(GeneratorSpecification const & g, std::string const & outputDir, [[maybe_unused]] std::string const & nspace) {
    if (g.args.size() != 3)
        throw std::runtime_error("Invalid number or arguments, usage: sine, FILENAME, N");
    std::string filename{g.args[0]};
    unsigned n = std::atoi(g.args[1].c_str());
    int max = std::atoi(g.args[2].c_str());
    std::ofstream f(STR(outputDir << "/" << filename));
    f << "// DO NOT EDIT THIS FILE, IT HAS BEEN AUTOGENERATED BY generate-assets" << Writer::endl;
    f << "// sine table (length << " << n << ", max " << max << ")" << Writer::endl;
    for (unsigned i = 0; i < n; ++i) {
        if (i % 8 == 0)
            f << Writer::endl;
        // ensure the last value is max
        if (i == n - 1)
            f << max << ", ";
        else 
            f << static_cast<unsigned>(std::sin(i / (n * 2.0) * M_PI) * max) << ", ";  
    }
}

/** Generates raw opus data from provided input. 
 
    
 */
void generateRawOpus(GeneratorSpecification const & g, std::string const & outputDir, [[maybe_unused]] std::string const & nspace) {
    if (g.args.size() != 4)
        throw std::runtime_error("Invalid number or arguments, usage: rawopus, filename, samplerate, channels, bitrate");
    std::string filename{g.args[0]};
    unsigned sr = std::atoi(g.args[1].c_str());
    unsigned channels = std::atoi(g.args[2].c_str());
    unsigned bitrate = std::atoi(g.args[3].c_str());
    // read it all, memory is cheap on big computers
    std::vector<int16_t> samples;
    std::ifstream input(filename, std::ios::binary | std::ios::ate);
    if (!input.good())
        throw std::runtime_error(STR("Error opening file " << filename));
    std::streamsize fileSize = input.tellg();
    input.seekg(0, std::ios::beg);
    size_t numElements = fileSize / sizeof(int16_t);
    samples.resize(numElements);
    if (!input.read(reinterpret_cast<char*>(samples.data()), fileSize))
        throw std::runtime_error(STR("Error reading file " << filename));
    input.close();   
    // calculate frame size, 
    namespace opus = rckid::opus;
    namespace audio = rckid::audio;
    unsigned frameSize = rckid::opus::frameSize(static_cast<audio::SampleRate>(sr), static_cast<audio::Channels>(channels), opus::FrameSize::ms20);
    while (samples.size() % frameSize != 0)
        samples.push_back(0);
    std::cout << "Loaded " << samples.size() << " samples from input file, frame size " << frameSize << " samples" << std::endl;
    uint8_t buffer[4096];
    opus::Encoder enc{static_cast<audio::SampleRate>(sr), static_cast<audio::Channels>(channels), bitrate};
    size_t i = 0;
    size_t encoded = 0;
    std::string outputPath = STR(outputDir + "/audio/opus");
    std::string className = STR(std::filesystem::path(filename).stem().c_str());
    std::filesystem::create_directories(outputPath);
    std::ofstream out{STR(outputPath + "/" + className + ".h")};
    out << "#pragma once" << Writer::endl << Writer::endl;
    out << "// DO NOT EDIT THIS FILE, IT HAS BEEN AUTOGENERATED BY generate-assets" << Writer::endl << Writer::endl;
    out << "#include <rckid/audio/opus.h>" << Writer::endl << Writer::endl;
    out << "namespace " << nspace << "::audio::opus {" << Writer::endl << Writer::endl;
    out << "    class " << className << "{" << Writer::endl;
    out << "        static constexpr audio::SampleRate sampleRate = audio::SampleRate::khz" << (sr/1000) << ";" << Writer::endl;
    out << "        static constexpr audio::Channels channels = audio::Channels::" << ((channels == 1) ? "Mono" : "Stereo") << ";" << Writer::endl;
    out << "        static constexpr uint32_t bitrate = " << bitrate << ";" << Writer::endl;
    out << "        static constexpr uint32_t frameSize = " << frameSize << ";" << Writer::endl << Writer::endl;
    out << "        static constexpr uint8_t data[] = {" << Writer::endl;

    while (i < samples.size()) {
        size_t frameLength = enc.encodePacket(samples.data() + i, frameSize, buffer, 4096);
        encoded += frameLength + 1;
        out << "            " << frameLength << ",";
        for (size_t i = 0; i < frameLength; ++i)
            out << (unsigned)buffer[i] << ",";
        out << Writer::endl;
        i += frameSize;        
    }
    out << "        }; // data" << Writer::endl;
    out << "    }; // class " << className << Writer::endl;
    out << "} // namesapce " << nspace << "::audio::opus" << Writer::endl;
    std::cout << "File encoded in " << encoded << " bytes" << std::endl;
}




