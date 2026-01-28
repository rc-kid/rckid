#include <FatFS/ff.h>
#include <FatFS/diskio.h>

#include <littlefs/lfs.h>

#include <rckid/filesystem.h>

// ================================================================================================
// FatFS device driver (using SD card)
// ================================================================================================

extern "C" {

    DSTATUS disk_status(BYTE pdrv) {
        ASSERT(pdrv == 0);
        return rckid::hal::fs::sdCapacityBlocks() != 0 ? RES_OK : STA_NODISK;
    }

    DSTATUS disk_initialize(BYTE pdrv) {
        ASSERT(pdrv == 0);
        return rckid::hal::fs::sdCapacityBlocks() != 0 ? RES_OK : STA_NODISK;
    }

    DRESULT disk_read(BYTE pdrv, BYTE * buff, LBA_t sector, UINT count) {
        ASSERT(pdrv == 0);
        if (! rckid::hal::fs::sdCapacityBlocks() != 0)
            return RES_NOTRDY;
        rckid::hal::fs::sdReadBlocks(static_cast<uint32_t>(sector), buff, count);
        return RES_OK;
    }

    DRESULT disk_write(BYTE pdrv, BYTE const * buff, LBA_t sector, UINT count) {
        ASSERT(pdrv == 0);
        if (! rckid::hal::fs::sdCapacityBlocks())
            return RES_NOTRDY;
        rckid::hal::fs::sdWriteBlocks(static_cast<uint32_t>(sector), buff, count);
        return RES_OK;
    }

    DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void * buff) {
        ASSERT(pdrv == 0);
        if (!rckid::hal::fs::sdCapacityBlocks())
            return RES_NOTRDY;
        switch (cmd) {
            // no need to do anything for CTRL_SYNC as the FatFS exposed API is blocking
            case CTRL_SYNC:
                break;
            // returns the number of SD card blocks
            case GET_SECTOR_COUNT:
                *(LBA_t *)buff = rckid::hal::fs::sdCapacityBlocks();
                break;
            case GET_BLOCK_SIZE:
                *(DWORD *)buff = 1;
                break;
            // Sector size is fixed to 512 bytes so no need to set anything here
            case GET_SECTOR_SIZE:
            // CTRL_TRIM is not supported, all other ioctls are not supported as well
            case CTRL_TRIM:
            default:
                return RES_PARERR;
        }
        return RES_OK;
    }

    DWORD get_fattime() {
        return 0;
    }

    // lfs wrappers for the RCKid cartridge interface

    int lfs_device_read(lfs_config const * c, lfs_block_t block, lfs_off_t off, void * buffer, lfs_size_t size) {
        rckid::hal::fs::cartridgeRead(block * c->block_size + off, reinterpret_cast<uint8_t *>(buffer), size);
        return 0;
    }

    int lfs_device_write(lfs_config const * c, lfs_block_t block, lfs_off_t off, void const * buffer, lfs_size_t size) {
        ASSERT(size == rckid::hal::fs::cartridgeWriteSizeBytes());
        rckid::hal::fs::cartridgeWrite(block * c->block_size + off, reinterpret_cast<uint8_t const *>(buffer), size);
        return 0;
    }

    int lfs_device_erase(lfs_config const * c, lfs_block_t block) {
        rckid::hal::fs::cartridgeErase(static_cast<uint32_t>(block * c->block_size));
        return 0;
    }

    int lfs_device_sync([[maybe_unused]] lfs_config const * c) {
        return 0;
    }
}



namespace rckid::fs {

    namespace {
        FATFS * fs_ = nullptr;
        lfs_t lfs_;
        lfs_config lfsCfg_;
    } // anonymous namespace

    class FatFSFileReader : public RandomReadStream {
    public:

        ~FatFSFileReader() override {
            f_close(& f_);
        }

        uint32_t read(uint8_t * buffer, uint32_t bufferSize) override {
            UINT bytesRead = 0;
            f_read(& f_, buffer, bufferSize, & bytesRead);
            return bytesRead;
        }

        bool eof() const override {
            return f_eof(& f_);
        }

        uint32_t size() const override {
            return static_cast<uint32_t>(f_size(&f_));
        }

        uint32_t seek(uint32_t position) override {
            f_lseek(& f_, position);
            return static_cast<uint32_t>(f_.fptr);
        }

        uint32_t tell() const override {
            return static_cast<uint32_t>(f_.fptr);
        }

    private:

        friend unique_ptr<RandomReadStream> readFile(String const & path, Drive dr);

        FIL f_;
    }; // fs::FatFSFileReader

    class FatFSFileWriter : public rckid::RandomWriteStream {
    public:
        ~FatFSFileWriter() override {
            f_close(& f_);
        }

        uint32_t tryWrite(uint8_t const * buffer, uint32_t numBytes) override {
            UINT bytesWritten = 0; 
            f_write(& f_, buffer, numBytes, & bytesWritten);
            return bytesWritten;
        }

        uint32_t seek(uint32_t position) override {
            f_lseek(& f_, position);
            return static_cast<uint32_t>(f_.fptr);
        }

        uint32_t tell() const override {
            return static_cast<uint32_t>(f_.fptr);
        }

    private:

        friend unique_ptr<RandomWriteStream> writeFile(String const & path, Drive dr);
        friend unique_ptr<RandomWriteStream> appendFile(String const & path, Drive dr);

        FIL f_;
    }; // fs::FatFSFileWriter

    class LittleFSFileReader : public RandomReadStream {
    public:
        ~LittleFSFileReader() override {
            lfs_file_close(& lfs_, & f_);
        }

        uint32_t read(uint8_t * buffer, uint32_t bufferSize) override {
            return lfs_file_read(& lfs_, & f_, buffer, bufferSize);
        }

        bool eof() const override {
            return lfs_file_tell(& lfs_, & f_) == lfs_file_size(& lfs_, & f_);
        }

        uint32_t size() const override {
            return lfs_file_size(& lfs_, & f_);
        }

        uint32_t seek(uint32_t position) override {
            lfs_file_seek(& lfs_, & f_, position, 0);
            return lfs_file_tell(& lfs_, & f_);
        }

        uint32_t tell() const override {
            return lfs_file_tell(& lfs_, & f_);        
        }
        
    private:

        friend unique_ptr<RandomReadStream> readFile(String const & path, Drive dr);

        mutable lfs_file_t f_;
    }; // LittleFSFileReader

    class LittleFSFileWriter : public rckid::RandomWriteStream {
    public:
        ~LittleFSFileWriter() override {
            lfs_file_close(& lfs_, & f_);
        }

        uint32_t tryWrite(uint8_t const * buffer, uint32_t numBytes) override {
            return lfs_file_write(& lfs_, & f_, buffer, numBytes);
        }

        uint32_t seek(uint32_t position) override {
            lfs_file_seek(& lfs_, & f_, position, 0);
            return lfs_file_tell(& lfs_, & f_);
        }

        uint32_t tell() const override {
            return lfs_file_tell(& lfs_, & f_);        
        }

    private:

        friend unique_ptr<RandomWriteStream> writeFile(String const & path, Drive dr);
        friend unique_ptr<RandomWriteStream> appendFile(String const & path, Drive dr);

        mutable lfs_file_t f_;

    }; // LittleFSFileWriter

    // path manipulation functions

    String stem(String const & path) {
        if (path.empty())
            return path;
        size_t end = path.size();
        size_t i = end - 1;
        for (; i > 0; --i) {
            if (path[i] == '.')
                end = i;
            else if (path[i] == '/')
                break;
        }
        if (path[i] == '/')
            ++i;
        return path.substr(i, end - i);
    }

    String ext(String const & path) {
        if (path.size() > 1) {    
            for (size_t i = path.size() - 1; i > 0; --i) {
                if (path[i] == '.')
                    return path.substr(i);
            }
        }
        return "";
    }

    String join(String const & path, String const & item) {
        if (path.endsWith('/'))
            return STR(path << item);
        else
            return STR(path << "/" << item);
    }

    String parent(String const & path) {
        for (size_t i = path.size() - 1; i > 0; --i) {
            if (path[i] == '/')
                return path.substr(0, i);
        }
        return "/";
    }

    String root(String const & path) {
        if (path.empty())
            return path;
        uint32_t i = 0;
        if (path[0] == '/')
            ++i; 
        while (i < path.size() && path[i] != '/') 
            ++i;
        return path.substr(0, i);
    }

    // filesystem functions
#ifndef RCKID_CUSTOM_FILESYSTEM

    void initializeFilesystem() {
        memset(& lfs_, 0, sizeof(lfs_t));
        memset(& lfsCfg_, 0, sizeof(lfs_config));
        // initialize LittleFS settings for the cartridge
        // block device operations
        lfsCfg_.read  = lfs_device_read;
        lfsCfg_.prog  = lfs_device_write;
        lfsCfg_.erase = lfs_device_erase;
        lfsCfg_.sync  = lfs_device_sync;
        // block device configuration
        lfsCfg_.read_size = 1;
        lfsCfg_.prog_size = hal::fs::cartridgeWriteSizeBytes();
        lfsCfg_.block_size = hal::fs::cartridgeEraseSizeBytes();
        lfsCfg_.block_count = hal::fs::cartridgeCapacityBytes() / lfsCfg_.block_size;
        lfsCfg_.cache_size = hal::fs::cartridgeWriteSizeBytes();
        lfsCfg_.lookahead_size = 32;
        lfsCfg_.block_cycles = 500;
        // finally, try mounting the filesystems if possible
        // if the capacity is non-zero, mount the cartridge filesystem
        if (hal::fs::cartridgeCapacityBytes() != 0) {
            if (! mount(Drive::Cartridge)) {
                format(Drive::Cartridge);
                mount(Drive::Cartridge);
            }
        }
        // also mount the SD card if present
        if (hal::fs::sdCapacityBlocks() != 0)
            mount(Drive::SD);


    }

    bool isMounted(Drive dr) {
        switch (dr) {
            case Drive::SD:
                return fs_ != nullptr;
            case Drive::Cartridge:
                return lfs_.cfg != nullptr;
        }
        UNREACHABLE;
    }

    bool format(Drive dr) {
        switch (dr) {
            case Drive::SD: {
                MKFS_PARM opts;
                opts.fmt = FM_EXFAT;
                opts.n_fat = 0; 
                opts.align = 0;
                opts.n_root = 0;
                opts.au_size = 0;
                BYTE work[FF_MAX_SS];
                return f_mkfs("", & opts, work, FF_MAX_SS) == FR_OK;
            }
            case Drive::Cartridge: {
                int ok = lfs_format(&lfs_, & lfsCfg_);
                lfs_.cfg = nullptr;
                return ok == 0;
            }
        }
        UNREACHABLE;
    } 

    bool mount(Drive dr) {
        switch (dr) {
            case Drive::SD:
                if (fs_ != nullptr) {
                    LOG(LL_WARN, "SD already mounted");
                    return true;
                }
                fs_ = new FATFS();
                if (f_mount(fs_, "", /* mount immediately */ 1) != FR_OK) {
                    delete fs_;
                    fs_ = nullptr;
                    return false;
                }
                return true;
            case Drive::Cartridge:
                if (lfs_.cfg == & lfsCfg_) {
                    LOG(LL_WARN, "Cartridge already mounted");
                    return true;
                }
                if (lfs_mount(&lfs_, &lfsCfg_) != 0) {
                    lfs_.cfg = nullptr;
                    return false;
                }
                return true;
        }
        UNREACHABLE;
    }

    void unmount(Drive dr) {
        switch (dr) {
            case Drive::SD:
                f_unmount("");
                delete fs_;
                fs_ = nullptr;
                break;
            case Drive::Cartridge:
                lfs_unmount(&lfs_);
                lfs_.cfg = nullptr;
                break;
        }
        UNREACHABLE;
    }

    uint64_t getCapacity(Drive dr) {
        if (!isMounted(dr))
            return 0;
        switch (dr) {
            case Drive::SD:
                return static_cast<uint64_t>(fs_->n_fatent - 2) * fs_->csize * 512;
            case Drive::Cartridge:
                return lfsCfg_.block_count * lfsCfg_.block_size;
        }
        UNREACHABLE;
    }

    uint64_t getFreeCapacity(Drive dr) {
        if (!isMounted(dr))
            return 0;
        switch (dr) {
            case Drive::SD: {
                DWORD n;
                FATFS * fs;
                f_getfree("", & n, &fs);
                return static_cast<uint64_t>(n) * fs_->csize * 512;
            }
            case Drive::Cartridge: {
                int allocatedBlocks = lfs_fs_size(&lfs_);
                if (allocatedBlocks < 0)
                    return 0; // error
                return (lfsCfg_.block_count - allocatedBlocks) * lfsCfg_.block_size;
            }
        }
        UNREACHABLE;
    }

    Filesystem getFormat(Drive dr) {
        if (!isMounted(dr))
            return Filesystem::Unrecognized;
        switch (dr) {
            case Drive::SD: 
                switch (fs_->fs_type) {
                    case FS_FAT16:
                        return Filesystem::FAT16;
                    case FS_FAT32:
                        return Filesystem::FAT32;
                    case FS_EXFAT:
                        return Filesystem::exFAT;
                    default:
                        return Filesystem::Unrecognized;
                }
            case Drive::Cartridge:
                return Filesystem::LittleFS;
        }
        UNREACHABLE;
    }

    String getLabel(Drive dr) {
        if (!isMounted(dr))
            return "";
        switch (dr) {
            case Drive::SD: {
                char * label = new char[13];
                memset8(reinterpret_cast<uint8_t*>(label), ' ', 12);
                label[12] = '\0';
                f_getlabel("",label, 0);
                return String{immutable_ptr<char>{label}};
            }
            case Drive::Cartridge:
                return "Cartridge";
        }
        UNREACHABLE;
    }

    bool exists(String const & path, Drive dr) {
        if (!isMounted(dr))
            return false;
        switch (dr) {
            case Drive::SD: {
                FILINFO f;
                if (f_stat(path.c_str(), & f) != FR_OK)
                    return false;
                return f.fname[0] != 0;        
            }
            case Drive::Cartridge: {
                lfs_info info;
                return lfs_stat(&lfs_, path.c_str(), & info) == 0;
            }
        }
        UNREACHABLE;
    }

    bool isFolder(String const & path, Drive dr) {
        if (!isMounted(dr))
            return false;
        switch (dr) {
            case Drive::SD: {
                FILINFO f;
                if (f_stat(path.c_str(), & f) != FR_OK)
                    return false;
                return f.fname[0] != 0 && (f.fattrib & AM_DIR);        
            }
            case Drive::Cartridge: {
                lfs_info info;
                if (lfs_stat(&lfs_, path.c_str(), & info) != 0)
                    return false;
                return info.type & LFS_TYPE_DIR;
            }
        }
        UNREACHABLE;
    }

    bool isFile(String const & path, Drive dr) {
        if (!isMounted(dr))
            return false;
        switch (dr) {
            case Drive::SD: {
                FILINFO f;
                if (f_stat(path.c_str(), & f) != FR_OK)
                    return false;
                return f.fname[0] != 0 && ! (f.fattrib & AM_DIR);        
            }
            case Drive::Cartridge: {
                lfs_info info;
                if (lfs_stat(&lfs_, path.c_str(), & info) != 0)
                    return false;
                return info.type == LFS_TYPE_REG;
            }
        }
        UNREACHABLE;
    }

    bool createFolder(String const & path, Drive dr) {
        if (!isMounted(dr))
            return false;
        switch (dr) {
            case Drive::SD:
                return f_mkdir(path.c_str()) == FR_OK;
            case Drive::Cartridge:
                return lfs_mkdir(&lfs_, path.c_str()) == 0;
        }
        UNREACHABLE;
    }

    bool createFolders(String const & path, Drive dr) {
        if (!isMounted(dr))
            return false;
        if (path == nullptr || path[0] == 0)
            return false;
        uint32_t i = 0;
        if (path[0] == '/') 
            ++i;
        while (true) {
            if (path[i] == '/' || path[i] == 0) {
                String p = path.substr(i);
                if (! isFolder(p.c_str(), dr) &&  (! createFolder(p.c_str(), dr)))
                    return false;
                if (path[i] == 0)
                    break;
            }
            ++i;
        }
        return true;
    }

    bool eraseFile(String const & path, Drive dr) {
        if (!isMounted(dr))
            return false;
        switch (dr) {
            case Drive::SD:
                return f_unlink(path.c_str()) == FR_OK;
            case Drive::Cartridge:
                return lfs_remove(&lfs_, path.c_str()) == 0;
        }
        UNREACHABLE;
    }

    unique_ptr<RandomReadStream> readFile(String const & path, Drive dr) {
        if (! isMounted(dr))
            return nullptr;
        switch (dr) {
            case Drive::SD: {
                auto result = new FatFSFileReader();
                if (f_open(& result->f_, path.c_str(), FA_READ) == FR_OK)
                    return unique_ptr<RandomReadStream>{result};
                delete result;
                return nullptr;
            }
            case Drive::Cartridge: {
                auto result = new LittleFSFileReader();
                if (lfs_file_open(& lfs_, & result->f_, path.c_str(), LFS_O_RDONLY) >= 0)
                    return unique_ptr<RandomReadStream>{result};
                delete result;
                return nullptr;
            }
        }
        UNREACHABLE;
    }

    unique_ptr<RandomWriteStream> writeFile(String const & path, Drive dr) {
        if (! isMounted(dr))
            return nullptr;
        switch (dr) {
            case Drive::SD: {
                auto result = new FatFSFileWriter();
                if (f_open(& result->f_, path.c_str(), FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
                    return unique_ptr<RandomWriteStream>{result};
                delete result;
                return nullptr;
            }
            case Drive::Cartridge: {
                auto result = new LittleFSFileWriter();
                if (lfs_file_open(& lfs_, & result->f_, path.c_str(), LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) >= 0)
                    return unique_ptr<RandomWriteStream>{result};
                delete result;
                return nullptr;
            }
        }
        UNREACHABLE;
    }

    unique_ptr<RandomWriteStream> appendFile(String const & path, Drive dr) {
        if (! isMounted(dr))
            return nullptr;
        switch (dr) {
            case Drive::SD: {
                auto result = new FatFSFileWriter();
                if (f_open(& result->f_, path.c_str(), FA_WRITE | FA_OPEN_APPEND) == FR_OK)
                    return unique_ptr<RandomWriteStream>{result};
                delete result;
                return nullptr;
            }
            case Drive::Cartridge: {
                auto result = new LittleFSFileWriter();
                if (lfs_file_open(& lfs_, & result->f_, path.c_str(), LFS_O_WRONLY | LFS_O_APPEND) >= 0)
                    return unique_ptr<RandomWriteStream>{result};
                delete result;
                return nullptr;
            }
        }
        UNREACHABLE;
    }

    uint32_t readFolder(String const & path, Drive dr, std::function<void(FolderEntry const &)> callback) {
        if (! isMounted(dr))
            return 0;
        uint32_t count = 0;
        switch (dr) {
            case Drive::SD: {
                DIR dir;
                FILINFO fno;
                if (f_opendir(& dir, path.c_str()) != FR_OK)
                    return 0;
                while (true) {
                    if (f_readdir(& dir, & fno) != FR_OK || fno.fname[0] == 0)
                        break;
                    FolderEntry entry;
                    entry.name = String{fno.fname};
                    entry.isFolder = (fno.fattrib & AM_DIR) != 0;
                    entry.size = static_cast<uint32_t>(fno.fsize);
                    callback(entry);
                    ++count;
                }
                f_closedir(& dir);
                break;
            }
            case Drive::Cartridge: {
                lfs_dir_t dir;
                if (lfs_dir_open(&lfs_, &dir, path.c_str()) != 0)
                    return 0;
                while (true) {
                    lfs_info info;
                    int res = lfs_dir_read(&lfs_, &dir, &info);
                    if (res < 0 || res == 0)
                        break;
                    FolderEntry entry;
                    entry.name = String{info.name};
                    entry.isFolder = (info.type & LFS_TYPE_DIR) != 0;
                    entry.size = static_cast<uint32_t>(info.size);
                    callback(entry);
                    ++count;
                }
                lfs_dir_close(&lfs_, &dir);
                break;
            }
        }
        return count;
    }

#endif // RCKID_CUSTOM_FILESYSTEM
}