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

    static constexpr Drive Device = Drive::Device;
    static constexpr Drive Cartridge = Drive::Cartridge;

    enum class Format {
        Unrecognized,
        FAT12, 
        FAT16, 
        FAT32, 
        EXFAT,
        LittleFS,
    };

    /** Returns the label associated with the device. 
     */
    std::string getLabel(Drive drive = Drive::Device);

    /** Returns the format of the given device. 
        
        Not really important from the user's perspective as all formats are abstracted by the filesystem API.  
     */
    Format getFormat(Drive drive = Drive::Device);
    uint64_t getTotalCapacity(Drive drive = Drive::Device);

    /** Returns the available free capacity on the device.
     
        Depending on the underlying filesystem and its size, this can take time. 
     */
    uint64_t getFreeCapacity(Drive drive = Drive::Device);


    /** File that supports basic reading & writing. 
     
        TODO also add async read support with callbacks. 
     */
    class File {
    public:
        static File openRead(std::string const & path, Drive drive = Drive::Device);
        static File openWrite(std::string const & path, Drive drive = Drive::Device);

        File(File const &) = delete;
        File(File && from):
            drive_{from.drive_}, 
            pimpl_{from.pimpl_} {
            from.pimpl_ = nullptr;
        }

        // delete copy ctor, enable move ctor

        ~File();

    private: 
        File(Drive drive, void * impl): drive_{drive}, pimpl_{impl} {}
        Drive drive_;
        void * pimpl_;

    }; // rckid::fs::File


    /** Simple folder elements iterator. 
     */
    class Folder {
    public:

        static Folder open(std::string const & path, Drive drive = Drive::Device);

        Folder(Folder const &) = delete;
        Folder(Folder && from):
            drive_{from.drive_}, 
            pimpl_{from.pimpl_} {
            from.pimpl_ = nullptr;
        }

        ~Folder();

    private:

        Folder(Drive drive, void * impl): drive_{drive}, pimpl_{impl} {} 
        Drive drive_;
        void * pimpl_;

    }; // rckid::fs::Folder


} // namespace rckid