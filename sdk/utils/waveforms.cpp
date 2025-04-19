#define _USE_MATH_DEFINES
#include <cmath>

#include <platform.h>
#include <platform/args.h>

#include "assets_utils.h"

namespace fs = std::filesystem;

/** Generates waveform tables for sine, triangular, sawtooth and square waves of different duty cycles.
 */
int main(int argc, char const * argv[]) {
    Args::Arg<uint32_t> values{0};
    Args::Arg<std::string> outputFile{""};
    Args::Arg<std::string> ns{"namespace", "rckid::assets"};
    Args::Arg<uint32_t> max{"max", 32767};
    Args::parse(argc, argv, { values, outputFile, ns, max});
    std::ofstream ofile{outputFile.value()};
    if (!ofile.good())
        throw std::runtime_error(STR("Unable to open output file " << outputFile.value()));
    generateAssetsFileHeader(argc, argv, ofile);
    std::string indent = "";
    if (! ns.value().empty()) {
        ofile << "namespace " << ns.value() << " {" << std::endl << std::endl;
        indent = "    ";
    }
    ofile << indent << "/** Full of sine wave from 0 to " << max.value() << " with " << values.value() << " values." << std::endl;
    ofile << indent << " */" << std::endl;
    ofile << indent << "constexpr int16_t WaveformSin[] = {";
    for (size_t i = 0, e = values.value(); i < e; ++i) {
        if (i % 8 == 0)
            ofile << std::endl << indent << "    ";
        ofile <<  static_cast<int16_t>(std::sin(i * M_PI * 2 / e) * max.value()) << ", ";
    }
    ofile << std::endl << indent << "}; // WaveformSin" << std::endl << std::endl;

    ofile << indent << "/** Triangle wave " << max.value() << " with " << values.value() << " values." << std::endl;
    ofile << indent << " */" << std::endl;
    ofile << indent << "constexpr int16_t WaveformTriangle[] = {";
    for (size_t i = 0, e = values.value(); i < e; ++i) {
        if (i % 8 == 0)
            ofile << std::endl << indent << "    ";
        int x = i * max.value() * 2 / e;
        if (x > max.value())
            x -= max.value() * 2;
        ofile << static_cast<int16_t>(x) << ", ";
    }
    ofile << std::endl << indent << "}; // WaveformTriangle" << std::endl << std::endl;

    ofile << indent << "/** Sawtooth wave " << max.value() << " with " << values.value() << " values." << std::endl;
    ofile << indent << " */" << std::endl;
    ofile << indent << "constexpr int16_t WaveformSawtooth[] = {";
    for (size_t i = 0, e = values.value() / 4; i < e; ++i) {
        if (i % 8 == 0)
            ofile << std::endl << indent << "    ";
        int x = i * max.value() / e;
        ofile << static_cast<int16_t>(x) << ", ";
    }
    for (size_t i = 0, e = values.value() / 4; i < e; ++i) {
        if (i % 8 == 0)
            ofile << std::endl << indent << "    ";
        int x = max.value() - i * max.value() / e;
        ofile << static_cast<int16_t>(x) << ", ";
    }
    for (size_t i = 0, e = values.value() / 4; i < e; ++i) {
        if (i % 8 == 0)
            ofile << std::endl << indent << "    ";
        int x = i * max.value() / e;
        ofile << static_cast<int16_t>(-x) << ", ";
    }
    for (size_t i = 0, e = values.value() / 4; i < e; ++i) {
        if (i % 8 == 0)
            ofile << std::endl << indent << "    ";
        int x = max.value() - i * max.value() / e;
        ofile << static_cast<int16_t>(-x) << ", ";
    }
    ofile << std::endl << indent << "}; // WaveformSawtooth" << std::endl << std::endl;

    if (! ns.value().empty())
        ofile << "} // namespace " << ns.value() << std::endl;
    return EXIT_SUCCESS;
}