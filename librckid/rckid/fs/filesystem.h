#pragma once

#include <string>

namespace rckid::fs {

    /** RCKid supports two drives, the Device drive which is the SD card inside the device and a Cartridge drive, which is a portion of the ROM inside cartridge, if used. 

        TODO LittleFS is not supported yet 
     
     */
    enum class Drive {
        Device, 
        Cartridge,
    }; 

    enum class Format {
        Unrecognized,
        FAT12, 
        FAT16, 
        FAT32, 
        EXFAT,
    };

    static constexpr Drive Device = Drive::Device;
    static constexpr Drive Cartridge = Drive::Cartridge;

    std::string getLabel(Drive drive = Drive::Device);
    Format getFormat(Drive drive = Drive::Device);
    uint64_t getTotalCapacity(Drive drive = Drive::Device);
    uint64_t getFreeCapacity(Drive drive = Drive::Device);


    class Folder {
    public:

        static Folder open(std::string const & path);


    }; // rckid::fs::Folder


    class File {

    }; // rckid::fs::File

} // namespace rckid