#include <filesystem>

#include <platform.h>
#include <platform/args.h>

#include <rckid/graphics/color.h>

#include "assets_utils.h"


::Color adjustToRGB(::Color c) {
    float a = c.a / 255.0f;
    return {
        (unsigned char)(c.r * a),
        (unsigned char)(c.g * a),
        (unsigned char)(c.b * a),
        c.a
    };
}

void convertSingleFile(std::string input, std::string output) {
    std::filesystem::create_directories(std::filesystem::path{output}.parent_path());
    std::ofstream ofile(output, std::ios::binary);
    // load the image
    Image img = LoadImage(input.c_str());
    for (int x = img.width - 1; x >= 0; --x) {
        for (int y = 0; y < img.height; ++y) {
            ::Color pixel = adjustToRGB(GetImageColor(img, x, y));
            uint16_t raw = rckid::Color::RGB(pixel.r, pixel.g, pixel.b).toRGB565();  
            ofile.write(reinterpret_cast<char*>(&raw), sizeof(raw));
        }
    }
    uint16_t width = img.width;
    uint16_t height = img.height;
    ofile.write(reinterpret_cast<char*>(&width), sizeof(width));
    ofile.write(reinterpret_cast<char*>(&height), sizeof(height));
    UnloadImage(img);
}

/** Takes image, loads it and converts it to the raw format rom bitmap. 
 
    Usage: 

        raw-image-converter IMAGE_FILE OUTPUT_FILE
    
    If directory is given instead of input file, then directory as output is expected and *all* files in the input directory will be converted. 
 */

int main(int argc, char const * argv[]) {
    Args::Arg<std::string> inputFile{""};
    Args::Arg<std::string> outputFile{""};
    Args::parse(argc, argv, { inputFile, outputFile});
    if (std::filesystem::is_directory(inputFile.value())) {
        for (const auto & entry : std::filesystem::directory_iterator(inputFile.value())) {
            if (entry.is_regular_file()) {
                std::string inputPath = entry.path().string();
                std::string outputPath = outputFile.value() + "/" + entry.path().stem().string() + ".raw";
                convertSingleFile(inputPath, outputPath);
            }
        }
    } else {
        convertSingleFile(inputFile.value(), outputFile.value());
    }
   return EXIT_SUCCESS;
}