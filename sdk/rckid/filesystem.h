#pragma once
#include "FatFS/ff.h"
#include "littlefs/lfs.h"

#include "rckid.h"
#include "utils/stream.h"

namespace rckid::filesystem {

    /** RCKid supports two filesystems, SD card and a pieces of the flash memory inside cartridge. 
     */
    enum class Drive {
        SD,
        Cartridge, 
        Invalid,
    }; 
    
    /** Possible filesystem formats understood by the SDK. 
     */
    enum class Filesystem {
        FAT12,
        FAT16,
        FAT32,
        exFAT,
        LittleFS, 
        Unrecognized
    };

    /** File with read-only access. 
     */
    class FileRead : public RandomReadStream {
    public:
        uint32_t size() const override;

        uint32_t seek(uint32_t position) override;

        uint32_t read(uint8_t * buffer, uint32_t numBytes) override;

        ~FileRead() override;

        FileRead(FileRead && from):
            drive_{from.drive_} {
            switch (drive_) {
                case Drive::SD:
                    sd_ = from.sd_;
                    break;
                case Drive::Cartridge:
                    cart_ = from.cart_;
                    break;
                default:
                    break;
            }
            from.drive_ = Drive::Invalid;
        }

    private:

        friend FileRead fileRead(char const * filename, Drive dr);

        FileRead():
            drive_{Drive::Invalid} {
        }
        Drive drive_;
        union {
            FIL sd_;
            lfs_file_t cart_;
        };
    }; 

    /** File with write access. 
     */
    class FileWrite : public WriteStream {
    public:

        uint32_t write(uint8_t const * buffer, uint32_t numBytes) override;

        ~FileWrite() override;

        FileWrite(FileWrite && from):
            drive_{from.drive_} {
            switch (drive_) {
                case Drive::SD:
                    sd_ = from.sd_;
                    break;
                case Drive::Cartridge:
                    cart_ = from.cart_;
                    break;
                default:
                    break;
            }
            from.drive_ = Drive::Invalid;
        }

    private:

        friend FileWrite fileWrite(char const * filename, Drive dr);
        friend FileWrite fileAppend(char const * filename, Drive dr);

        FileWrite():
            drive_{Drive::Invalid} {
        }
        Drive drive_;
        union {
            FIL sd_;
            lfs_file_t cart_;
        };
    };



    /** Formats the drive. 
     
        For the SD card, formats the card using the ExFAT filesystem, while LittleFS is used for cartridge's flash memory where wear levelling is important. 

        NOTE that the drive can only be formatted when not mounted. 
     */
    bool format(Drive dr = Drive::SD); 

    /** Mounts the SD card. 
     
        Returns true if the card was mounted (i.e. the card is present and its format has been understood), false otherwise (no SD card or corrupted data). 
     */
    bool mount(Drive dr = Drive::SD);

    /** Unmounts previously mounted SD card. 
     
        No-op if the card is currently not mounted. 
     */
    void unmount(Drive dr = Drive::SD);

    /** Returns the capacity of the mounted SD card in bytes. 
     */
    uint64_t getCapacity();

    /** Returns the available free capacity on the device.
     
        Depending on the underlying filesystem and its size, this can take time. 
     */
    uint64_t getFreeCapacity();  

    /** Returns the filesystem used on the SD card. 

        Not very important from the user's perspective as all the formats are abstracted away.  
     */
    Filesystem getFormat();

    /** Returns the SD Card label. 
     */
    std::string getLabel(); 

    /** Returns true if the given path exists (i.e. either directory, or file).
     */
    bool exists(char const * path);

    /** Returns true if  the given path is a valid file. 
     */
    bool isDir(char const * path);

    /** Returns true if the given path is a valid directory. 
     */
    bool isFile(char const * path); 


    /** Opens given file for reading. 
     */
    FileRead fileRead(char const * filename, Drive dr = Drive::SD);

    FileWrite fileWrite(char const * filename, Drive dr = Drive::SD);

    FileWrite fileAppend(char const * filename, Drive dr = Drive::SD);


} // namespace rckid::filesystem