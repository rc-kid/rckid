#include <platform.h>
#include <platform/args.h>

#include "assets_utils.h"

namespace fs = std::filesystem;

int main(int argc, char const * argv[]) {
    Args::Arg<std::string> inputFolder{""};
    Args::Arg<std::string> outputFile{""};
    Args::Arg<std::string> ns{"namespace", "rckid::assets"};
    Args::Arg<std::string> rawPath{"raw-path", ""};
    Args::parse(argc, argv, { inputFolder, outputFile, ns, rawPath});
    // get the folder contents
    std::vector<FolderItem> items;
    for (auto const & entry : fs::directory_iterator{inputFolder.value()})
        items.push_back(FolderItem{entry});
    // 
    fs::path rawFolder = fs::path{outputFile.value()}.parent_path() / rawPath.value();
    fs::create_directories(rawFolder);
    std::string rawPrefix = (rawPath.value().empty()) ? std::string{} : (rawPath.value() + "/");
    // output them 
    std::ofstream ofile{outputFile.value()};
    if (!ofile.good())
        throw std::runtime_error(STR("Unable to open output file " << outputFile.value()));
    generateAssetsFileHeader(argc, argv, ofile);
    std::string indent = "";
    if (! ns.value().empty()) {
        ofile << "namespace " << ns.value() << " {" << std::endl << std::endl;
        indent = "    ";
    }
    for (FolderItem const & i : items) {
        ofile << indent << "// File: " << i.filename << std::endl;
        ofile << indent << "// Size: " << i.bytes.size() << std::endl;
        ofile << indent << "static constexpr uint8_t " << i.className << "[] = {" << std::endl;
        ofile << indent << "    #include \"" << rawPrefix << i.className << ".inc.h\"" << std::endl;
        ofile << indent << "};" << std::endl << std::endl;
        std::ofstream rawFile(rawFolder / (i.className + ".inc.h"));
        rawFile << "// File: " << i.filename << std::endl;
        rawFile << "// Size: " << i.bytes.size() << std::endl;
        i.outputBytes(rawFile, "");
    }
    if (! ns.value().empty())
        ofile << "} // namespace " << ns.value() << std::endl;
    return EXIT_SUCCESS;

} 