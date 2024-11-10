#pragma once
#include "FatFS/ff.h"
#include "littlefs/lfs.h"

#include "rckid.h"
#include "utils/stream.h"

namespace rckid::filesystem {

    /** RCKid supports two filesystems, SD card and a pieces of the flash memory inside cartridge. 
     */
    enum class Drive {
        SD = 1,
        Cartridge = 2, 
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

        bool good() const { return drive_ != 0; }

        Drive drive() const { 
            ASSERT(drive_ != 0);
            return static_cast<Drive>(drive_); 
        }

        uint32_t size() const override;

        uint32_t seek(uint32_t position) override;

        uint32_t read(uint8_t * buffer, uint32_t numBytes) override;

        ~FileRead() override;

        FileRead(FileRead && from):
            drive_{from.drive_} {
            switch (drive_) {
                case static_cast<unsigned>(Drive::SD):
                    sd_ = from.sd_;
                    break;
                case static_cast<unsigned>(Drive::Cartridge):
                    cart_ = from.cart_;
                    break;
                default:
                    break;
            }
            from.drive_ = 0;
        }

    private:

        friend FileRead fileRead(char const * filename, Drive dr);

        FileRead() = default;

        unsigned drive_ = 0;
        union {
            FIL sd_;
            lfs_file_t cart_;
        };
    }; 

    /** File with write access. 
     */
    class FileWrite : public WriteStream {
    public:

        bool good() const { return drive_ != 0; }

        Drive drive() const { 
            ASSERT(drive_ != 0);
            return static_cast<Drive>(drive_); 
        }

        uint32_t write(uint8_t const * buffer, uint32_t numBytes) override;

        ~FileWrite() override;

        FileWrite(FileWrite && from):
            drive_{from.drive_} {
            switch (drive_) {
                case static_cast<unsigned>(Drive::SD):
                    sd_ = from.sd_;
                    break;
                case static_cast<unsigned>(Drive::Cartridge):
                    cart_ = from.cart_;
                    break;
                default:
                    break;
            }
            from.drive_ = 0;
        }

    private:

        friend FileWrite fileWrite(char const * filename, Drive dr);
        friend FileWrite fileAppend(char const * filename, Drive dr);

        FileWrite() = default;

        unsigned drive_ = 0;
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

    /** Mounts the specified drive. 
     
        Returns true if the drive was mounted (i.e. the card is present and its format has been understood, or cartridge has allocated filesystem space and has been formatted correctly), false otherwise (no SD card or corrupted data). 
     */
    bool mount(Drive dr = Drive::SD);

    /** Unmounts previously mounted drive.
     
        No-op if the drive is currently not mounted. 
     */
    void unmount(Drive dr = Drive::SD);

    /** Returns whether the given drive is already mounted. 
     */
    bool isMounted(Drive dr = Drive::SD);

    /** Returns the capacity of the selected drive in bytes. 
     */
    uint64_t getCapacity(Drive dr = Drive::SD);

    /** Returns the available free capacity on the drive.
     
        Depending on the underlying filesystem and its size, this can take time and shouldnot be overly trusted as it inly represents the best effort. 
     */
    uint64_t getFreeCapacity(Drive dr = Drive::SD);  

    /** Returns the filesystem used on the given drive.

        Not very important from the user's perspective as all the formats are abstracted away. Always returns LittleFS for the cartridge drive.
     */
    Filesystem getFormat(Drive dr = Drive::SD);

    /** Returns the drive label. 
      
        For SD card it's the filesystem label, but for the cartridge returns always "Cartridge" (labels are not supported by the cartridge filesystem .
     */
    std::string getLabel(Drive dr = Drive::SD); 

    /** Returns true if the given path exists (i.e. either directory, or file).
     */
    bool exists(char const * path, Drive dr = Drive::SD);

    /** Returns true if  the given path is a valid file. 
     */
    bool isDir(char const * path, Drive dr = Drive::SD);

    /** Returns true if the given path is a valid directory. 
     */
    bool isFile(char const * path, Drive dr = Drive::SD); 


    /** Opens given file for reading. 
     */
    FileRead fileRead(char const * filename, Drive dr = Drive::SD);

    FileWrite fileWrite(char const * filename, Drive dr = Drive::SD);

    FileWrite fileAppend(char const * filename, Drive dr = Drive::SD);


} // namespace rckid::filesystem