#include <cmath>

#include <FatFS/ff.h>
#include <FatFS/diskio.h>

#include "filesystem.h"

// ================================================================================================
// FatFS device driver (using SD card)
// ================================================================================================

extern "C" {

    DSTATUS disk_status(BYTE pdrv) {
        ASSERT(pdrv == 0);
        return rckid::sdCapacity() != 0 ? RES_OK : STA_NODISK;
    }

    DSTATUS disk_initialize(BYTE pdrv) {
        ASSERT(pdrv == 0);
        return rckid::sdCapacity() != 0 ? RES_OK : STA_NODISK;
    }

    DRESULT disk_read(BYTE pdrv, BYTE * buff, LBA_t sector, UINT count) {
        ASSERT(pdrv == 0);
        if (! rckid::sdCapacity() != 0)
            return RES_NOTRDY;
        return rckid::sdReadBlocks(static_cast<uint32_t>(sector), buff, count) ? RES_OK : RES_ERROR;
    }

    DRESULT disk_write(BYTE pdrv, BYTE const * buff, LBA_t sector, UINT count) {
        ASSERT(pdrv == 0);
        if (! rckid::sdCapacity())
            return RES_NOTRDY;
        return rckid::sdWriteBlocks(static_cast<uint32_t>(sector), buff, count) ? RES_OK : RES_ERROR;
    }

    DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void * buff) {
        ASSERT(pdrv == 0);
        if (!rckid::sdCapacity())
            return RES_NOTRDY;
        switch (cmd) {
            // no need to do anything for CTRL_SYNC as the FatFS exposed API is blocking
            case CTRL_SYNC:
                break;
            // returns the number of SD card blocks
            case GET_SECTOR_COUNT:
                *(LBA_t *)buff = rckid::sdCapacity();
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
        rckid::cartridgeRead(block * c->block_size + off, reinterpret_cast<uint8_t *>(buffer), size);
        return 0;
    }

    int lfs_device_write(lfs_config const * c, lfs_block_t block, lfs_off_t off, void const * buffer, lfs_size_t size) {
        ASSERT(size == rckid::cartridgeWriteSize());
        rckid::cartridgeWrite(block * c->block_size + off, reinterpret_cast<uint8_t const *>(buffer));
        return 0;
    }

    int lfs_device_erase(lfs_config const * c, lfs_block_t block) {
        rckid::cartridgeErase(static_cast<uint32_t>(block * c->block_size));
        return 0;
    }

    int lfs_device_sync([[maybe_unused]] lfs_config const * c) {
        return 0;
    }
}

namespace rckid::fs {

    namespace {
        [[maybe_unused]]
        FATFS * fs_ = nullptr;
        lfs_t lfs_;
        lfs_config lfsCfg_;
#if RCKID_ENABLE_HOST_FILESYSTEM
        std::filesystem::path sdRoot_;
        std::filesystem::path cartridgeRoot_;
        bool sdMounted_ = false;
        bool cartridgeMounted_ = false;
#endif
    }

    void initialize() {
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
        lfsCfg_.prog_size = cartridgeWriteSize();
        lfsCfg_.block_size = cartridgeEraseSize();
        lfsCfg_.block_count = cartridgeCapacity() / lfsCfg_.block_size;
        lfsCfg_.cache_size = cartridgeWriteSize();
        lfsCfg_.lookahead_size = 32;
        lfsCfg_.block_cycles = 500;
        // if the capacity is non-zero, mount the cartridge filesystem
        if (cartridgeCapacity() != 0) {
            if (! mount(Drive::Cartridge)) {
                format(Drive::Cartridge);
                mount(Drive::Cartridge);
            }
        }
        // also mount the SD card if present
        if (sdCapacity() != 0)
            mount(Drive::SD);
    }

#if RCKID_ENABLE_HOST_FILESYSTEM
    void initialize(std::string const & sdRoot, std::string const & cartridgeRoot) {
        if (std::filesystem::is_directory(sdRoot)) {
            sdRoot_ = sdRoot;
            mount(Drive::SD);
        } else {
            LOG(LL_ERROR, "SD root path is not a directory: " << sdRoot);
        }
        if (std::filesystem::is_directory(cartridgeRoot)) {
            cartridgeRoot_ = cartridgeRoot;
            mount(Drive::Cartridge);
        } else {
                LOG(LL_ERROR, "Cartridge root path is not a directory: " << cartridgeRoot);
        }
    }

    std::filesystem::path getHostPath(Drive dr, char const * path) {
        std::filesystem::path result{path[0] == '/' ? path + 1 : path};
        return (dr == Drive::SD) ? sdRoot_ / result : cartridgeRoot_ / result;
    }
#endif    

    uint32_t FileRead::size() const {
#if RCKID_ENABLE_HOST_FILESYSTEM
        if (host_ == nullptr)
            return 0;
        std::streampos currentPos = host_->tellg();
        host_->seekg(0, std::ios::end);
        std::streamsize size = host_->tellg();
        host_->seekg(currentPos);
        return static_cast<uint32_t>(size);
#else
        switch (drive_) {
            case static_cast<unsigned>(Drive::SD):
               return static_cast<uint32_t>(f_size(&sd_));
            case static_cast<unsigned>(Drive::Cartridge):
                return lfs_file_size(& lfs_, const_cast<lfs_file_t*>(& cart_));
            default:
                return 0;
        }
#endif
    }

    uint32_t FileRead::seek(uint32_t position) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        ASSERT(host_ != nullptr);
        host_->seekg(position, std::ios::beg);
        std::streamsize size = host_->tellg();
        return static_cast<uint32_t>(size);
#else
        switch (drive_) {
            case static_cast<unsigned>(Drive::SD):
                f_lseek(& sd_, position);
                return static_cast<uint32_t>(sd_.fptr);
            case static_cast<unsigned>(Drive::Cartridge):
                lfs_file_seek(& lfs_, & cart_, position, 0);
                return lfs_file_tell(& lfs_, & cart_);
            default:
                ASSERT(false); // seeking invalid file is not allowed
        }
#endif
    }

    uint32_t FileRead::tell() const {
#if RCKID_ENABLE_HOST_FILESYSTEM
        ASSERT(host_ != nullptr);
        std::streamsize size = host_->tellg();
        return static_cast<uint32_t>(size);
#else
        switch (drive_) {
            case static_cast<unsigned>(Drive::SD):
                return static_cast<uint32_t>(sd_.fptr);
            case static_cast<unsigned>(Drive::Cartridge):
                return lfs_file_tell(& lfs_, & cart_);
            default:
                ASSERT(false); // telling position of invalid file is not allowed
        }
#endif
    }

    uint32_t FileRead::read(uint8_t * buffer, uint32_t numBytes) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        ASSERT(host_ != nullptr);
        host_->read(reinterpret_cast<char *>(buffer), numBytes);
        return static_cast<uint32_t>(host_->gcount());
#else
        switch (drive_) {
                case static_cast<unsigned>(Drive::SD): {
                    UINT bytesRead = 0;
                    f_read(& sd_, buffer, numBytes, & bytesRead);
                    return bytesRead;
                }
                case static_cast<unsigned>(Drive::Cartridge):
                    return lfs_file_read(& lfs_, & cart_, buffer, numBytes);
                default:
                    ASSERT(false); // trying to read from invalid file
            }
#endif
    }

    bool FileRead::eof() const {
#if RCKID_ENABLE_HOST_FILESYSTEM
        if (host_ == nullptr)
            return true;

        return host_->tellg() == fileLength_;
#else
        switch (drive_) {
            case static_cast<unsigned>(Drive::SD):
                return f_eof(& sd_);
            case static_cast<unsigned>(Drive::Cartridge):
                return lfs_file_tell(& lfs_, & cart_) == lfs_file_size(& lfs_, & cart_);
            default:
                return true;
        }
#endif          
    }

    void FileRead::close() {
#if RCKID_ENABLE_HOST_FILESYSTEM
        // if file does not exist, host is nullptr
        if (host_ != nullptr) {
            ASSERT(host_ != nullptr);
            delete host_;
            host_ = nullptr;
        }
#else
        switch (drive_) {
            case static_cast<unsigned>(Drive::SD):
                f_close(& sd_);
                break;
            case static_cast<unsigned>(Drive::Cartridge):
                lfs_file_close(& lfs_, & cart_);
                break;
            default:
                break;
        }
#endif
    }

    // FileWrite 

    void FileWrite::close() {
#if RCKID_ENABLE_HOST_FILESYSTEM
        delete host_;
        host_ = nullptr;
#else
        switch (drive_) {
            case static_cast<unsigned>(Drive::SD):
                f_close(& sd_);
                break;
            case static_cast<unsigned>(Drive::Cartridge):
                lfs_file_close(& lfs_, & cart_);
                break;
            default:
                break;
        }
#endif
    }

    uint32_t FileWrite::write(uint8_t const * buffer, uint32_t numBytes) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        ASSERT(host_ != nullptr);
        host_->write(reinterpret_cast<char const *>(buffer), numBytes);
        return numBytes;
#else
        switch (drive_) {
            case static_cast<unsigned>(Drive::SD): {
                UINT bytesWritten = 0; 
                f_write(& sd_, buffer, numBytes, & bytesWritten);
                return bytesWritten;
            }
            case static_cast<unsigned>(Drive::Cartridge):
                return lfs_file_write(& lfs_, & cart_, buffer, numBytes);
            default:
                ASSERT(false); // writing invalid file
        }
#endif
    }

    // Folder

    Folder::~Folder() {
#if RCKID_ENABLE_HOST_FILESYSTEM
        delete host_;
#else
        switch(drive_) {
            case static_cast<unsigned>(Drive::SD):
                f_closedir(& sd_);
                break;
            case static_cast<unsigned>(Drive::Cartridge):
                lfs_dir_close(& lfs_, & cart_);
                break;
            default:
                break;
        }
#endif
    }

    void Folder::rewind() {
#if RCKID_ENABLE_HOST_FILESYSTEM
        SystemMallocGuard g;
        delete host_;
        try {
            host_ = new std::filesystem::directory_iterator{hostPath_};
        } catch (...) {
            LOG(LL_ERROR, "Failed to rewind folder: " << hostPath_.c_str());
            drive_ = 0;
            host_ = nullptr;
        }
#else
        switch(drive_) {
            case static_cast<unsigned>(Drive::SD):
                f_rewinddir(& sd_);
                break;
            case static_cast<unsigned>(Drive::Cartridge):
                lfs_dir_rewind(& lfs_, & cart_);
                break;
            default:
                UNREACHABLE;
        }
#endif
    }

    void Folder::readNext(Entry & into) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        ASSERT(host_ != nullptr);
        SystemMallocGuard g;
        if (*host_ == std::filesystem::directory_iterator{}) {
            into.drive_ = 0; // invalidate the entry
            into.host_ = Entry::HostFileInfo{};
            return;
        }
        into.drive_ = drive_;
        auto const & entry = *host_;
        into.host_.isFile = entry->is_regular_file();
        into.host_.size = entry->is_regular_file() ? static_cast<uint32_t>(entry->file_size()) : 0;
        into.host_.name = entry->path().filename().c_str();
        into.host_.path = entry->path().c_str();
        ++(*host_);
#else
        into.drive_ = 0; // invalidate the entry
        switch (drive_) {
            case static_cast<unsigned>(Drive::SD):
                if (f_readdir(& sd_, & into.sd_) == FR_OK && into.sd_.fname[0] != 0)
                    into.drive_ = drive_;
                break;
            case static_cast<unsigned>(Drive::Cartridge): {
                while (lfs_dir_read(& lfs_, & cart_, & into.cart_) > 0) {
                    // skip . and .. folders to be compatible fith fatfs
                    if (strcmp(".", into.cart_.name) == 0 || strcmp("..", into.cart_.name) == 0)
                        continue;
                    into.drive_ = drive_;
                    break;                    
                }
                break;
            }
            default:
                UNREACHABLE;
        }
#endif
    }

    bool format(Drive dr) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        try {
            SystemMallocGuard g_;
            switch (dr) {
                case Drive::SD:
                    if (sdRoot_.empty())
                        return false;
                    std::filesystem::remove_all(sdRoot_);
                    break;
                case Drive::Cartridge:
                    if (cartridgeRoot_.empty())
                        return false;
                    std::filesystem::remove_all(cartridgeRoot_);
                    break;
                default:
                    return false;
            }
            return true;
        } catch (...) {
            return false;
        }
#else        
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
            default:
                UNREACHABLE;
        }
#endif
    }

    bool mount(Drive dr) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        switch (dr) {
            case Drive::SD:
                if (sdRoot_.empty())
                    return false;
                sdMounted_ = true;
                return true;
            case Drive::Cartridge:
                if (cartridgeRoot_.empty())
                    return false;
                cartridgeMounted_ = true;
                return true;
            default:
                UNREACHABLE;
        }
#else
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
            default:
                UNREACHABLE;
        }
#endif
    }

    void unmount(Drive dr) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        switch (dr) {
            case Drive::SD:
                sdMounted_ = false;
                break;
            case Drive::Cartridge:
                cartridgeMounted_ = false;
                break;
            default:
                UNREACHABLE;
        }
#else
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
            default:
                UNREACHABLE;
        }
#endif
    }

    bool isMounted(Drive dr) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        switch (dr) {
            case Drive::SD:
                return sdMounted_;
            case Drive::Cartridge:
                return cartridgeMounted_;
            default:
                UNREACHABLE;
        }
#else
        switch (dr) {
            case Drive::SD:
                return fs_ != nullptr;
            case Drive::Cartridge:
                return lfs_.cfg != nullptr;
            default:
                UNREACHABLE;
        }
#endif
    }

    uint64_t getCapacity(Drive dr) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        try {
            SystemMallocGuard g_;
            switch (dr) {
                case Drive::SD:
                    if (sdRoot_.empty())
                        return 0;
                    return std::filesystem::space(sdRoot_).capacity;
                case Drive::Cartridge:
                    if (cartridgeRoot_.empty())
                        return 0;
                    return std::filesystem::space(cartridgeRoot_).capacity;
                default:
                    return 0;
            }
        } catch (...) {
            return 0;
        }
#else        
        if (!isMounted(dr))
            return 0;
        switch (dr) {
            case Drive::SD:
                return static_cast<uint64_t>(fs_->n_fatent - 2) * fs_->csize * 512;
            case Drive::Cartridge:
                return lfsCfg_.block_count * lfsCfg_.block_size;
            default:
                UNREACHABLE;
        }
#endif
    }

    uint64_t getFreeCapacity(Drive dr) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        try {
            SystemMallocGuard g_;
            switch (dr) {
                case Drive::SD:
                    if (sdRoot_.empty())
                        return 0;
                    return std::filesystem::space(sdRoot_).available;
                case Drive::Cartridge:
                    if (cartridgeRoot_.empty())
                        return 0;
                    return std::filesystem::space(cartridgeRoot_).available;
                default:
                    return 0;
            }
        } catch (...) {
            return 0;
        }
#else        
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
            default:
                UNREACHABLE;
        }
#endif
    }

    Filesystem getFormat(Drive dr) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        switch (dr) {
            case Drive::SD:
                if (! sdRoot_.empty())
                    return Filesystem::exFAT;
                break;
            case Drive::Cartridge:
                if (! cartridgeRoot_.empty())
                    return Filesystem::LittleFS;
                break;
            default:
                break;
        }
        return Filesystem::Unrecognized;
#else        
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
            default:
                UNREACHABLE;
        }
#endif
    }

    String getLabel(Drive dr) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        switch (dr) {
            case Drive::SD: {
                return sdRoot_.c_str();
            }
            case Drive::Cartridge:
                return cartridgeRoot_.c_str();
            default:
                UNREACHABLE;
        }
#else        
        if (!isMounted(dr))
            return "";
        switch (dr) {
            case Drive::SD: {
                String result{' ', 12};
                f_getlabel("",result.data(), 0);
                return result;
            }
            case Drive::Cartridge:
                return "Cartridge";
            default:
                UNREACHABLE;
        }
#endif
    }

    bool exists(char const * path, Drive dr) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        SystemMallocGuard g;
        std::filesystem::path p{getHostPath(dr, path)};
        return std::filesystem::exists(p);
#else
        if (!isMounted(dr))
            return false;
        switch (dr) {
            case Drive::SD: {
                FILINFO f;
                if (f_stat(path, & f) != FR_OK)
                    return false;
                return f.fname[0] != 0;        
            }
            case Drive::Cartridge: {
                lfs_info info;
                return lfs_stat(&lfs_, path, & info) == 0;
            }
            default:
                UNREACHABLE;
        }
#endif
    }

    bool isFolder(char const * path, Drive dr) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        SystemMallocGuard g;
        std::filesystem::path p{getHostPath(dr, path)};
        return std::filesystem::is_directory(p);
#else
        if (!isMounted(dr))
            return false;
        switch (dr) {
            case Drive::SD: {
                FILINFO f;
                if (f_stat(path, & f) != FR_OK)
                    return false;
                return f.fname[0] != 0 && (f.fattrib & AM_DIR);        
            }
            case Drive::Cartridge: {
                lfs_info info;
                if (lfs_stat(&lfs_, path, & info) != 0)
                    return false;
                return info.type & LFS_TYPE_DIR;
            }
            default:
                UNREACHABLE;
        }
#endif
    }

    bool isFile(char const * path, Drive dr) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        SystemMallocGuard g;
        std::filesystem::path p{getHostPath(dr, path)};
        return std::filesystem::is_regular_file(p);
#else
        if (!isMounted(dr))
            return false;
        switch (dr) {
            case Drive::SD: {
                FILINFO f;
                if (f_stat(path, & f) != FR_OK)
                    return false;
                return f.fname[0] != 0 && ! (f.fattrib & AM_DIR);        
            }
            case Drive::Cartridge: {
                lfs_info info;
                if (lfs_stat(&lfs_, path, & info) != 0)
                    return false;
                return info.type == LFS_TYPE_REG;
            }
            default:
                UNREACHABLE;
        }
#endif
    }

    bool createFolder(char const * path, Drive dr) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        SystemMallocGuard g_;
        std::filesystem::path p{getHostPath(dr, path)};
        return std::filesystem::create_directory(p);
#else
        if (!isMounted(dr))
            return false;
        switch (dr) {
            case Drive::SD:
                return f_mkdir(path) == FR_OK;
            case Drive::Cartridge:
                return lfs_mkdir(&lfs_, path) == 0;
            default:
                UNREACHABLE;
        }
#endif
    }

    bool createFolders(char const * path, Drive dr) {
        if (!isMounted(dr))
            return false;
        if (path == nullptr || path[0] == 0)
            return false;
        uint32_t i = 0;
        if (path[0] == '/') 
            ++i;
        while (true) {
            if (path[i] == '/' || path[i] == 0) {
                String p{path, i};
                if (! isFolder(p.c_str(), dr) &&  (! createFolder(p.c_str(), dr)))
                    return false;
                if (path[i] == 0)
                    break;
            }
            ++i;
        }
        return true;
    }

    bool eraseFile(char const * path, Drive dr) {
#if RCKID_ENABLE_HOST_FILESYSTEM
        SystemMallocGuard g;
        std::filesystem::path p{getHostPath(dr, path)};
        return std::filesystem::remove(p);
#else
        if (!isMounted(dr))
            return false;
        switch (dr) {
            case Drive::SD:
                return f_unlink(path) == FR_OK;
            case Drive::Cartridge:
                return lfs_remove(&lfs_, path) == 0;
            default:
                UNREACHABLE;
        }
#endif
    }

    uint32_t hash(char const * path, Drive dr) {
        FileRead f = fileRead(path, dr); 
        if (!f.good())
            return 0;
        uint32_t hash = 0;
        uint32_t size = 0;
        uint8_t buffer[32];
        uint32_t o = 0;
        while (true) {
            uint32_t bl = f.read(buffer, sizeof(buffer));
            for (uint32_t i = 0; i < sizeof(buffer); ++i)
                hash = hash + (buffer[i] << (o++ % 24));
            size += bl;
            if (bl < sizeof(buffer))
                break;
        }
        hash += size;
        return hash;
    }

    // file read, write and append operations

    FileRead fileRead(char const * path, Drive dr) {
        FileRead result;
#if RCKID_ENABLE_HOST_FILESYSTEM
        SystemMallocGuard g;
        std::filesystem::path p{getHostPath(dr, path)};
        result.host_ = new std::ifstream{p, std::ios::binary};
        if (result.host_->is_open()) {
            result.drive_ = static_cast<unsigned>(dr);
            std::streampos currentPos = result.host_->tellg();
            result.host_->seekg(0, std::ios::end);
            result.fileLength_ = result.host_->tellg();
            result.host_->seekg(currentPos);
        } else {
            delete result.host_;
            result.host_ = nullptr;
        }
#else
        if (! isMounted(dr))
            return result;
        switch (dr) {
            case Drive::SD:
                if (f_open(& result.sd_, path, FA_READ) == FR_OK)
                    result.drive_ = static_cast<unsigned>(dr);
                break;
            case Drive::Cartridge:
                if (lfs_file_open(& lfs_, & result.cart_, path, LFS_O_RDONLY) >= 0)
                    result.drive_ = static_cast<unsigned>(dr);
                break;
            default:
                UNREACHABLE;
        }
#endif
        return result;
    }

    FileWrite fileWrite(char const * path, Drive dr) {
        FileWrite result;
#if RCKID_ENABLE_HOST_FILESYSTEM
        SystemMallocGuard g;
        std::filesystem::path p{getHostPath(dr, path)};
        result.host_ = new std::ofstream{p, std::ios::binary};
        if (result.host_->is_open())
            result.drive_ = static_cast<unsigned>(dr);
        else {
            delete result.host_;
            result.host_ = nullptr;
        }
#else
        if (! isMounted(dr))
            return result;
        switch (dr) {
            case Drive::SD:
                if (f_open(& result.sd_, path, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
                    result.drive_ = static_cast<unsigned>(dr);
                break;
            case Drive::Cartridge:
                if (lfs_file_open(& lfs_, & result.cart_, path, LFS_O_WRONLY | LFS_O_CREAT) >= 0)
                    result.drive_ = static_cast<unsigned>(dr);
                break;
            default:
                UNREACHABLE;
        }
#endif
        return result;
    }

    FileWrite fileAppend(char const * path, Drive dr) {
        FileWrite result;
#if RCKID_ENABLE_HOST_FILESYSTEM
        SystemMallocGuard g;
        std::filesystem::path p{getHostPath(dr, path)};
        result.host_ = new std::ofstream{p, std::ios::binary | std::ios::app};
        if (result.host_->is_open())
            result.drive_ = static_cast<unsigned>(dr);
        else {
            delete result.host_;
            result.host_ = nullptr;
        }
#else
        if (! isMounted(dr))
            return result;
        switch (dr) {
            case Drive::SD:
                if (f_open(& result.sd_, path, FA_WRITE | FA_OPEN_APPEND) == FR_OK)
                    result.drive_ = static_cast<unsigned>(dr);
                break;
            case Drive::Cartridge:
                if (lfs_file_open(& lfs_, & result.cart_, path, LFS_O_WRONLY | LFS_O_APPEND) >= 0)
                    result.drive_ = static_cast<unsigned>(dr);
                break;
            default:
                UNREACHABLE;
        }
#endif
        return result;
    }

    Folder folderRead(char const * path, Drive dr) {
        Folder result;
#if RCKID_ENABLE_HOST_FILESYSTEM
        SystemMallocGuard g;
        std::filesystem::path p{getHostPath(dr, path)};
        try {
            result.host_ = new std::filesystem::directory_iterator{p};
            result.drive_ = static_cast<unsigned>(dr);
            result.hostPath_ = p;
        } catch (...) {
            LOG(LL_ERROR, "Failed to open folder: " << p.c_str());
        }
#else
        if (! isMounted(dr))
            return result;
        switch (dr) {
            case Drive::SD:
                if (f_opendir(& result.sd_, path) == FR_OK)
                    result.drive_ = static_cast<unsigned>(dr);
                break;
            case Drive::Cartridge:
                if (lfs_dir_open(& lfs_, & result.cart_, path) >=0)
                    result.drive_ = static_cast<unsigned>(dr);
                break;
            default:
                UNREACHABLE;
        }
#endif
        return result;
    }

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

} // namespace rckid::fs