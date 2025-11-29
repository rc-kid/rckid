#pragma once

#include "FatFS/ff.h"
#include "littlefs/lfs.h"

#include "rckid.h"
#include "utils/stream.h"

namespace rckid::fs {

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
        Unrecognized,
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

        bool eof() const override;

        void close(); 

        ~FileRead() override { close(); }

        FileRead(FileRead && from) noexcept:
            drive_{from.drive_} {
#if RCKID_ENABLE_HOST_FILESYSTEM
            host_ = from.host_;
            fileLength_ = from.fileLength_;
            from.host_ = nullptr;
#else
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
#endif
            from.drive_ = 0;
        }

        FileRead() = default;

        FileRead & operator = (FileRead && from) noexcept {
            close();
            drive_ = from.drive_;
#if RCKID_ENABLE_HOST_FILESYSTEM
            delete host_;
            host_ = from.host_;
            from.host_ = nullptr;
#else
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
#endif
            from.drive_ = 0;
            return *this;
        }

    private:

        friend FileRead fileRead(char const * filename, Drive dr);

        unsigned drive_ = 0;
        union {
            FIL sd_;
            mutable lfs_file_t cart_;
#if RCKID_ENABLE_HOST_FILESYSTEM
            struct {
                std::ifstream * host_;
                int64_t fileLength_;
            };
#endif
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

        void close();

        uint32_t write(uint8_t const * buffer, uint32_t numBytes) override;

        ~FileWrite() override { close(); }

        FileWrite(FileWrite && from) noexcept:
            drive_{from.drive_} {
#if RCKID_ENABLE_HOST_FILESYSTEM
            host_ = from.host_;
            from.host_ = nullptr;
#else
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
#endif
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
#if RCKID_ENABLE_HOST_FILESYSTEM
            std::ofstream * host_;
#endif

        };
    };

    class Entry {
    public:

        Entry(Entry const & from):
            drive_{from.drive_} {
#if RCKID_ENABLE_HOST_FILESYSTEM
            SystemMallocGuard g;
            new (&host_) HostFileInfo{from.host_};
#else
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
#endif
        }
        Entry(Entry && from) noexcept:
            drive_{from.drive_} {
#if RCKID_ENABLE_HOST_FILESYSTEM
            new (&host_) HostFileInfo{std::move(from.host_)};
#else
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
#endif
        }

        ~Entry() {
#if RCKID_ENABLE_HOST_FILESYSTEM
            host_.~HostFileInfo();;
#endif
        }

        bool good() const { return drive_ != 0; }

        char const * name() const {
#if RCKID_ENABLE_HOST_FILESYSTEM
            return host_.name.c_str();
#else
            switch (drive_) {
                case static_cast<unsigned>(Drive::SD):
                    return sd_.fname;
                case static_cast<unsigned>(Drive::Cartridge):
                    return cart_.name;
                default:
                    return ""; // avoid nullptr
            }
#endif
        }

        uint32_t size() const {
#if RCKID_ENABLE_HOST_FILESYSTEM
            return host_.size;
#else
            switch (drive_) {
                case static_cast<unsigned>(Drive::SD):
                    return static_cast<uint32_t>(sd_.fsize);
                case static_cast<unsigned>(Drive::Cartridge):
                    return cart_.size;
                default:
                    return 0;
            }
#endif
        }

        bool isFile() const {
#if RCKID_ENABLE_HOST_FILESYSTEM
            return host_.isFile;
#else
            switch (drive_) {
                case static_cast<unsigned>(Drive::SD):
                    return (sd_.fattrib & AM_DIR) == 0;
                case static_cast<unsigned>(Drive::Cartridge):
                    return cart_.type == LFS_TYPE_REG;
                default:
                    return false;
            }
#endif
        }

        bool isFolder() const {
#if RCKID_ENABLE_HOST_FILESYSTEM
            return ! host_.isFile; 
#else
            switch (drive_) {
                case static_cast<unsigned>(Drive::SD):
                    return (sd_.fattrib & AM_DIR) != 0;
                case static_cast<unsigned>(Drive::Cartridge):
                    return cart_.type == LFS_TYPE_DIR;
                default:
                    return false;
            }
#endif
        }

        bool operator == (Entry const & other) const {
            if (drive_ != other.drive_)
                return false;
#if RCKID_ENABLE_HOST_FILESYSTEM
            return host_ == other.host_;
#else
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
#endif
        }

        bool operator != (Entry const & other) const {
            if (drive_ != other.drive_)
                return true;
#if RCKID_ENABLE_HOST_FILESYSTEM
            return host_ != other.host_;
#else
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
#endif
        }
    private:

#if RCKID_ENABLE_HOST_FILESYSTEM
        struct HostFileInfo {
            String name;
            String path;
            uint32_t size = 0;
            bool isFile = false;

            bool operator == (HostFileInfo const & other) const {
                return (path == other.path) && (size == other.size) && (isFile == other.isFile);
            }
            bool operator != (HostFileInfo const & other) const {
                return (path != other.path) || (size != other.size) || (isFile != other.isFile);
            }
        };
#endif

        friend class Folder;

#if RCKID_ENABLE_HOST_FILESYSTEM
        Entry(): host_{} {}
#else
        Entry() = default;
#endif

        unsigned drive_ = 0;

        union {
            FILINFO sd_;
            lfs_info cart_;
#if RCKID_ENABLE_HOST_FILESYSTEM
            HostFileInfo host_;
#endif
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

        Folder() {
#if RCKID_ENABLE_HOST_FILESYSTEM
            host_ = nullptr;
#endif
        }

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
#if RCKID_ENABLE_HOST_FILESYSTEM
            std::filesystem::directory_iterator * host_;
#endif            
        }; 
#if RCKID_ENABLE_HOST_FILESYSTEM
        std::filesystem::path hostPath_;
#endif
    }; // rckid::fs::Folder

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
    String getLabel(Drive dr = Drive::SD); 

    /** Returns true if the given path exists (i.e. either directory, or file).
     */
    bool exists(char const * path, Drive dr = Drive::SD);

    /** Returns true if  the given path is a valid file. 
     */
    bool isFolder(char const * path, Drive dr = Drive::SD);

    /** Returns true if the given path is a valid directory. 
     */
    bool isFile(char const * path, Drive dr = Drive::SD); 

    /** Creates given folder.
     */
    bool createFolder(char const * path, Drive dr = Drive::SD);

    /** Creates all folders in the given path that do not yet exist.
     */
    bool createFolders(char const * path, Drive dr = Drive::SD);

    /** Returns hash of given file. 
     */
    uint32_t hash(char const * path, Drive dr = Drive::SD);

    /** Opens given file for reading. 
     */
    FileRead fileRead(char const * path, Drive dr = Drive::SD);

    FileWrite fileWrite(char const * path, Drive dr = Drive::SD);

    FileWrite fileAppend(char const * path, Drive dr = Drive::SD);

    Folder folderRead(char const * path, Drive dr = Drive::SD);

    inline FileRead fileRead(String const & path, Drive dr = Drive::SD) { return fileRead(path.c_str(), dr); }
    inline FileWrite fileWrite(String const & path, Drive dr = Drive::SD) { return fileWrite(path.c_str(), dr); }
    inline Folder folderRead(String const & path, Drive dr = Drive::SD) { return folderRead(path.c_str(), dr); }

    /** Returns the path stem, i.e. the file, or last folder name without any extensions. 
     */
    String stem(String const & path);

    /** Returns the extension of the file or folder at given path. The path does not have to exist. 
     */
    String ext(String const & path);

    /** Joins two paths together. 
     */
    String join(String const & path, String const & item);

    /** Returns the parent folder of given path. If the path is file, returns the containing folder, otherwise returns the parent folder.
     */
    String parent(String const & path);

    /** Returns the first component of the path (i.e. the root folder). If the path has only one component, returns that component.
     */
    String root(String const & path);

} // namespace rckid::filesystem