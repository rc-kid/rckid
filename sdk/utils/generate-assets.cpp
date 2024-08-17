#include <filesystem>

#include "generate-assets.h"

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
        std::cout << "Generating assets from " << argv[1] << " to " << outputDir << std::endl;
        std::cout << "    namespace = " << nspace << std::endl;

        std::string generatorLine;
        size_t lineNum = 0;
        while (std::getline(configFile, generatorLine)) {
            ++lineNum;
            try {
                GeneratorSpecification g{generatorLine};
                if (g.name == "fontGlyphs") 
                    generateFontGlyphs(g, outputDir, nspace);
                else 
                    throw std::runtime_error("Unknown generator");
            } catch (std::exception const & e) {
                throw std::runtime_error(STR(e.what() << " at line " << lineNum << ":" << std::endl << generatorLine << std::endl));
            }
        }
        return EXIT_SUCCESS;
    } catch (std::exception const & e) {
        std::cout << "ERROR: " << e.what() << std::endl << std::endl;
        std::cout << "Usage: " << std::endl << std::endl;
        std::cout << "    generate-assets CONFIG_FILE OUTPUT_FOLDER NAMESPACE" << std::endl << std::endl;
        std::cout << "Where:" << std::endl;
        std::cout << "    CONFIG_FILE = file with description of what to generate" << std::endl;
        std::cout << "    OUTPUT_FOLDER = output folder where all cpp files with definitions will be stored" << std::endl;
        std::cout << "    NAMESPACE = namespace in which the assets will reside" << std::endl;
        return EXIT_FAILURE;
    }
        
}