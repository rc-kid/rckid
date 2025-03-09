#define _USE_MATH_DEFINES
#include <cmath>

#include <platform.h>
#include <platform/args.h>

#include "assets_utils.h"

namespace fs = std::filesystem;

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
    ofile << indent << "/** One quarter of sine wave from 0 to " << max.value() << " with " << values.value() << " values." << std::endl;
    ofile << indent << " */" << std::endl;
    ofile << indent << "constexpr int16_t SineTable[] = {";
    for (size_t i = 0, e = values.value(); i < e; ++i) {
        if (i % 8 == 0)
            ofile << std::endl << indent << "    ";
        ofile <<  static_cast<unsigned>(std::sin(i / (e * 2.0) * M_PI) * max.value()) << ", ";
    }
    ofile << std::endl << indent << "}; // SineTable" << std::endl;
    if (! ns.value().empty())
        ofile << "} // namespace " << ns.value() << std::endl;
    return EXIT_SUCCESS;
}