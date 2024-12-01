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

    inline char const * formatToStr(Filesystem f) {
        switch (f) {
            case Filesystem::FAT12:
                return "FAT12";
            case Filesystem::FAT16:
                return "FAT16";
            case Filesystem::FAT32:
                return "FAT32";
            case Filesystem::exFAT:
                return "exFAT";
            case Filesystem::LittleFS:
                return "LittleFS";
            case Filesystem::Unrecognized:
            default:
                return "Unrecognized";
        }
    }

    inline char const * driveToStr(Drive dr) {
        switch (dr) {
            case Drive::SD:
                return "sd";
            case Drive::Cartridge:
                return "cart";
            default:
                UNREACHABLE;
        }
    }

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

        void close(); 

        ~FileRead() override { close(); }

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

        FileRead() = default;

        FileRead & operator = (FileRead && from) {
            close();
            drive_ = from.drive_;
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
            return *this;
        }

    private:

        friend FileRead fileRead(char const * filename, Drive dr);

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

    class Entry {
    public:

        bool good() const { return drive_ != 0; }

        char const * name() const {
            switch (drive_) {
                case static_cast<unsigned>(Drive::SD):
                    return sd_.fname;
                case static_cast<unsigned>(Drive::Cartridge):
                    return cart_.name;
                default:
                    return ""; // avoid nullptr
            }
        }

        uint32_t size() const {
            switch (drive_) {
                case static_cast<unsigned>(Drive::SD):
                    return sd_.fsize;
                case static_cast<unsigned>(Drive::Cartridge):
                    return cart_.size;
                default:
                    return 0;
            }
        }

        bool isFile() const {
            switch (drive_) {
                case static_cast<unsigned>(Drive::SD):
                    return (sd_.fattrib & AM_DIR) == 0;
                case static_cast<unsigned>(Drive::Cartridge):
                    return cart_.type == LFS_TYPE_REG;
                default:
                    return false;
            }
        }

        bool isFolder() const {
            switch (drive_) {
                case static_cast<unsigned>(Drive::SD):
                    return (sd_.fattrib & AM_DIR) != 0;
                case static_cast<unsigned>(Drive::Cartridge):
                    return cart_.type == LFS_TYPE_DIR;
                default:
                    return false;
            }
        }

        bool operator == (Entry const & other) const {
            if (drive_ != other.drive_)
                return false;
            switch (drive_) {
                case static_cast<unsigned>(Drive::SD):
                    return (sd_.fsize == other.sd_.fsize) &&
                           (sd_.fdate == other.sd_.fdate) &&
                           (sd_.ftime == other.sd_.ftime) &&
                           (sd_.fattrib == other.sd_.fattrib) &&
                           strncmp(sd_.fname, other.sd_.fname, FF_LFN_BUF) == 0;
                case static_cast<unsigned>(Drive::Cartridge):
                    return (cart_.type == other.cart_.type) && 
                           (cart_.size == other.cart_.size) &&
                           strncmp(cart_.name, other.cart_.name, LFS_NAME_MAX) == 0;
                case 0:
                    // all invalid drives are equal and we already know both drives are the same
                    return true;
                default:
                    UNREACHABLE;
            }
        }

        bool operator != (Entry const & other) const {
            if (drive_ != other.drive_)
                return true;
            switch (drive_) {
                case static_cast<unsigned>(Drive::SD):
                    return (sd_.fsize != other.sd_.fsize) ||
                           (sd_.fdate != other.sd_.fdate) ||
                           (sd_.ftime != other.sd_.ftime) ||
                           (sd_.fattrib != other.sd_.fattrib) ||
                           strncmp(sd_.fname, other.sd_.fname, FF_LFN_BUF) != 0;
                case static_cast<unsigned>(Drive::Cartridge):
                    return (cart_.type != other.cart_.type) ||
                           (cart_.size != other.cart_.size) ||
                           strncmp(cart_.name, other.cart_.name, LFS_NAME_MAX) != 0;
                // if we get here and drive is 0 (invalid), then the other drive had to be 0 as well and all invalid entries are equal
                case 0:
                    return false;
                default:
                    UNREACHABLE;
            }
        }
        
    private:

        friend class Folder;

        unsigned drive_ = 0;

        union {
            FILINFO sd_;
            lfs_info cart_;
        };
    }; // Entry

    /** Simple folder listing  */
    class Folder {
    public:

        class Iterator {
        public:

            Entry & operator * () { return entry_; }
            Entry * operator -> () { return & entry_; }
            Iterator & operator ++ () {
                folder_->readNext(entry_);
                return *this;
            }

            bool operator == (Iterator const & other) const {
                return (entry_ == other.entry_) && (folder_ == other.folder_);
            }

            bool operator != (Iterator const & other) const {
                return (entry_ != other.entry_) || (folder_ != other.folder_);
            }

        private:

            friend class Folder;
            
            Iterator(Folder * folder): folder_(folder) {}
            Folder * folder_;
            Entry entry_;
        }; 

        ~Folder();

        bool good() const { return drive_ != 0; }

        Iterator begin() { 
            Iterator result{this};
            if (good()) {
                rewind();
                ++result;
            }
            return result;
        }

        Iterator end() { return Iterator{this}; }

    private:

        friend Folder folderRead(char const * path, Drive dr);

        void rewind();

        void readNext(Entry & into);

        unsigned drive_ = 0;
        union {
            DIR sd_;
            lfs_dir_t cart_;
        }; 

    }; // rckid::filesystem::Folder

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
    bool isFolder(char const * path, Drive dr = Drive::SD);

    /** Returns true if the given path is a valid directory. 
     */
    bool isFile(char const * path, Drive dr = Drive::SD); 

    /** Returns hash of given file. 
     */
    uint32_t hash(char const * path, Drive dr = Drive::SD);

    /** Opens given file for reading. 
     */
    FileRead fileRead(char const * path, Drive dr = Drive::SD);

    FileWrite fileWrite(char const * path, Drive dr = Drive::SD);

    FileWrite fileAppend(char const * path, Drive dr = Drive::SD);

    Folder folderRead(char const * path, Drive dr = Drive::SD);

    inline FileRead fileRead(std::string const & path, Drive dr = Drive::SD) { return fileRead(path.c_str(), dr); }
    inline FileWrite fileWrite(std::string const & path, Drive dr = Drive::SD) { return fileWrite(path.c_str(), dr); }
    inline Folder folderRead(std::string const & path, Drive dr = Drive::SD) { return folderRead(path.c_str(), dr); }

} // namespace rckid::filesystem