#include "generate-assets.h"

/** \defgroup assets Assets

    The SDK supports creating binary assets that can be included in the ROM files. This is done in a very simple way by converting the raw assets into C++ header files that define constexpr arrays with the information. Some extra asset types, such as font and tile definitions require slightly more processing. While the default SDK assets are always part of this repository, the asset generator utility can be used by cartridges to generate their own extra assets. 

    ## Asset Generator 

        generate-assets DEF_FILE OUTPUT_DIR NAMESPACE

    Reads the given asset DEF_FILE (see the types of assets below) and generates the assets specified therein into the provided OUTPUT_DIR. All assets will live in the provided C++ NAMESPACE. 
 
    ## Asset Types

    The asset types below can be described in the definition file, one asset type per line. The asset file format also supports blank lines and commented lines that start with `#`. Comments are only supported as the first character in a line. 

    ### Fonts

        fontGlyphs, PATH_TO_FONT, FONT_SIZE [, CLASS_NAME [, GLYPH_DEF_FILE ]]

    Generates the font definitions that can be used by the text drawing SDK primitives. Each font comprises of a set of glyphs that are stored as a 2 bpp column first left to right, top to bottom pixmaps. Unlike the font tiles described below, each glyph can have different size so that only the area actually used by the char is stored, saving memory.

    If the class name is omitted, the base name of the font followed by its size is expected. If glyph definition file is missing, glyphs for all printable characters (ASCII 32 - 127) will be generated. If specified, the glyph file is ecpected to be in the format used by the SDK, which is:

        GLYPH(GLYPH_NAME, CODEPOINT)

    (see the `symbol-glyphs-inc.h` file for etails)

    ### Folders

        folder, PATH_TO_FOLDER, NAMESPACE

    Takes all files in the folder and onverts their raw contents into byte arrays. Creates fle NAMESPACE.h in the output folder and in it one definition per file in the folder. The actual byte array contents will be stored in separate files (`raw/NAMESPACE/filename.h`) and included where appropriate. 

    ### Interpolation

        sine, FILENAME, N

    Generates sine table (1/4) from 0 to 65535 with N rows into the given filename. Generates only the contents of the table, which should then be manually inserted into code when needed as this varies greatly with different uses. 

    ### Tiles

    ### Font Tiles

    Special case of tiles where basis of the tiles are not images, but font glyphs. Unlike font glyphs, the tile support different bit depths (4, 8 or 16 compared to 2bpp for fonts) and they are all of the same size.  
 */

/** Generates assets for the RCKid. 
 
    Usage:

        generate-assets CONFIG_FILE OUTPUT_FOLDER NAMESPACE

    Where:

        CONFIG_FILE = file with description of what to generate
        OUTPUT_FOLDER = output folder where all cpp files with definitions will
        NAMESPACE = namespace in which the assets will reside 
    
 */
int main(int argc, char * argv []) {
    try {
        if (argc != 4)
            throw std::runtime_error("Invalid number of arguments");
        std::ifstream configFile{argv[1]};
        std::string outputDir{argv[2]};
        std::string nspace{argv[3]};
        std::cout << "Generating assets from " << argv[1] << " to " << outputDir << Writer::endl;
        std::cout << "    namespace = " << nspace << Writer::endl;

        std::string generatorLine;
        size_t lineNum = 0;
        while (std::getline(configFile, generatorLine)) {
            ++lineNum;
            // ignore comments and empty lines
            if (generatorLine.empty() || generatorLine[0] == '#')
                continue;
            try {
                GeneratorSpecification g{generatorLine};
                if (g.name == "fontGlyphs") 
                    generateFontGlyphs(g, outputDir, nspace);
                else if (g.name == "folder")
                    generateFolder(g, outputDir, nspace);
                else if (g.name == "sine")
                    generateSineTable(g, outputDir, nspace);
                else 
                    throw std::runtime_error("Unknown generator");
            } catch (std::exception const & e) {
                throw std::runtime_error(STR(e.what() << " at line " << lineNum << ":" << Writer::endl << generatorLine << Writer::endl));
            }
        }
        return EXIT_SUCCESS;
    } catch (std::exception const & e) {
        std::cout << "ERROR: " << e.what() << Writer::endl << Writer::endl;
        std::cout << "Usage: " << Writer::endl << Writer::endl;
        std::cout << "    generate-assets CONFIG_FILE OUTPUT_FOLDER NAMESPACE" << Writer::endl << Writer::endl;
        std::cout << "Where:" << Writer::endl;
        std::cout << "    CONFIG_FILE = file with description of what to generate" << Writer::endl;
        std::cout << "    OUTPUT_FOLDER = output folder where all cpp files with definitions will be stored" << Writer::endl;
        std::cout << "    NAMESPACE = namespace in which the assets will reside" << Writer::endl;
        return EXIT_FAILURE;
    }
        
}