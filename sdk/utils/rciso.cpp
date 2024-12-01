#include <cstdlib>
#include <iostream>
#include <fstream>

#include <rckid/filesystem.h>

namespace rckid {
    void initialize(bool createWindow);

    void enableSystemMalloc(bool value);
}


namespace fs = rckid::filesystem;

void showUsage() {
    std::cout << "rckid DRIVE COMMAND [args]" << std::endl << std::endl
              << "Where:" << std::endl << std::endl
              << "    DRIVE - drive to use (sd/cart)" << std::endl
              << "    COMMAND - command to use (see below)" << std::endl
              << "    [args] - optional arguments to the command (see bwlow) " << std::endl << std::endl
              << "Commands:" << std::endl << std::endl
              << "info" << std::endl
              << "    Shows information about the filesystem." << std::endl
              ;
}

void mount(fs::Drive dr) {
    if (!fs::mount(dr))
        throw std::runtime_error("Cannot mount required drive");
}

void info(fs::Drive dr) {
    ::mount(dr);
    std::cout << "Capacity:      " << fs::getCapacity(dr) << std::endl
              << "Free capacity: " << fs::getFreeCapacity(dr) << std::endl
              << "Format:        " << fs::formatToStr(fs::getFormat()) << std::endl
              << "Label:         " << fs::getLabel(dr) << std::endl;
}

void format(fs::Drive dr) {
    fs::unmount(dr);
    if (!fs::format(dr))
        throw std::runtime_error("Unable to format drive");
    std::cout << "Drive formatted" << std::endl;
}

void ls(fs::Drive dr, int argc, char * argv[]) {
    ::mount(dr);
    std::string path = "/";

    if (argc == 4)
        path = argv[3];
    else if (argc > 4)
        throw std::runtime_error("Invalid number of arguments to ls");

    std::cout << "Drive " << fs::driveToStr(dr) << ", folder " << path << std::endl << std::endl;
    fs::Folder f = fs::folderRead(path, dr);
    if (!f.good())
        throw std::runtime_error("Cannot read specified folder");
    
    for (auto const & i : f) {
        std::cout << i.name() << "  ";
        if (i.isFolder()) 
            std::cout << "DIR" << std::endl;
        else
            std::cout << i.size() << std::endl;
    }
}

void store(fs::Drive dr, int argc, char * argv[]) {
    ::mount(dr);
    if (argc != 5)
        throw std::runtime_error("Invalid number of arguments to store (store SRC DESC)");
    std::string src{argv[3]};
    std::string desc{argv[4]};
    std::cout << "Storing file " << src << " into " << desc << std::endl;

    rckid::enableSystemMalloc(true);
    std::ifstream input(src, std::ios::binary);
    if (!input.good())
        throw std::runtime_error("Unable to open input file");
    std::cout << "File opened" << std::endl;
    std::vector<char> bytes(
         (std::istreambuf_iterator<char>(input)),
         (std::istreambuf_iterator<char>()));
    input.close();
    std::cout << bytes.size() << " bytes read" << std::endl;
    rckid::enableSystemMalloc(false);

    fs::FileWrite f = fs::fileWrite(desc, dr);
    if (!f.good())
        throw std::runtime_error("Unable to open the destination file");
    
    // store the file now, is somewhat manageable chunks
    uint8_t * data = (uint8_t *)bytes.data();
    uint32_t numBytes = bytes.size();
    while (numBytes > 0) {
        uint32_t tx = f.write(data, (numBytes > 512) ? 512 : numBytes);
        numBytes -= tx;
        data += tx; 
    }
    std::cout << "Stored " << bytes.size() << " bytes" << std::endl;
}

int main(int argc, char * argv[]) {
    try {
        rckid::initialize(false);

        if (argc < 3)
            throw std::runtime_error("Invalid number of arguments");
        std::string driveName{argv[1]};
        fs::Drive dr;
        if (driveName == "sd")
            dr = fs::Drive::SD;
        else if (driveName == "cart")
            dr = fs::Drive::Cartridge;
        else
            throw std::runtime_error("Invalid drive value (only sd and cart are supported)");
        std::string cmd{argv[2]};
        if (cmd == "info")
            info(dr);
        else if (cmd == "format")
            ::format(dr);
        else if (cmd == "ls")
            ls(dr, argc, argv);
        else if (cmd == "store")
            store(dr, argc, argv);
        else
            throw std::runtime_error("Invalid command");
        return EXIT_SUCCESS;
    } catch (std::exception const & e) {
        std::cout << "Error: " << e.what() << std::endl << std::endl;
        showUsage();
        return EXIT_FAILURE;
    } catch (...) {
        std::cout << "Unknown error" << std::endl;
        showUsage();
        return EXIT_FAILURE;

    }
}