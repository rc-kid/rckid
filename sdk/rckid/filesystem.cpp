extern "C" {

#include <FatFS/ff.h>
#include <FatFS/diskio.h>

}

#include "rckid.h"
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
        return rckid::sdReadBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
    }

    DRESULT disk_write(BYTE pdrv, BYTE const * buff, LBA_t sector, UINT count) {
        ASSERT(pdrv == 0);
        if (! rckid::sdCapacity())
            return RES_NOTRDY;
        return rckid::sdWriteBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
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
        TRACE_LITTLEFS("Reading " << size << " from block " << block << ", offset " << off);
        rckid::cartridgeRead(block * c->block_size + off, reinterpret_cast<uint8_t *>(buffer), size);
        return 0;
    }

    int lfs_device_write(lfs_config const * c, lfs_block_t block, lfs_off_t off, void const * buffer, lfs_size_t size) {
        ASSERT(size == rckid::cartridgeWriteSize());
        TRACE_LITTLEFS("Writing " << size << " from block " << block << ", offset " << off);
        rckid::cartridgeWrite(block * c->block_size + off, reinterpret_cast<uint8_t const *>(buffer));
        return 0;
    }

    int lfs_device_erase(lfs_config const * c, lfs_block_t block) {
        TRACE_LITTLEFS("Erasing block " << block);
        rckid::cartridgeErase(static_cast<uint32_t>(block * c->block_size));
        return 0;
    }

    int lfs_device_sync([[maybe_unused]] lfs_config const * c) {
        TRACE_LITTLEFS("Sync");
        return 0;
    }
}

namespace rckid::filesystem {

    namespace {
        FATFS * fs_ = nullptr;

        lfs_t lfs_;
        lfs_config lfsCfg_;
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
            TRACE_LITTLEFS("Mounting cartridge flash storage");
            if (! mount(Drive::Cartridge)) {
                TRACE_LITTLEFS("Mount failed, formatting...");
                format(Drive::Cartridge);
                TRACE_LITTLEFS("Re-mounting after format");
                mount(Drive::Cartridge);
            }
        }
    }

    uint32_t FileRead::size() const {
        switch (drive_) {
            case static_cast<unsigned>(Drive::SD):
               return f_size(&sd_);
            case static_cast<unsigned>(Drive::Cartridge):
                return lfs_file_size(& lfs_, const_cast<lfs_file_t*>(& cart_));
            default:
                return 0;
        }
    }

    uint32_t FileRead::seek(uint32_t position) {
        switch (drive_) {
            case static_cast<unsigned>(Drive::SD):
                f_lseek(& sd_, position);
                return sd_.fptr;
            case static_cast<unsigned>(Drive::Cartridge):
                lfs_file_seek(& lfs_, & cart_, position, 0);
                return lfs_file_tell(& lfs_, & cart_);
            default:
                ASSERT(false); // seeking invalid file is not allowed
        }
    }

    uint32_t FileRead::read(uint8_t * buffer, uint32_t numBytes) {
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
    }

    FileRead::~FileRead() {
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
    }

    // FileWrite 

    uint32_t FileWrite::write(uint8_t const * buffer, uint32_t numBytes) {
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
    }

    FileWrite::~FileWrite() {
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
    }

    // Folder

    Folder::~Folder() {
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
    }

    void Folder::rewind() {
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
    }

    void Folder::readNext(Entry & into) {
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
            default:
                UNREACHABLE;
        }
    }

    bool mount(Drive dr) {
        switch (dr) {
            case Drive::SD:
                if (fs_ != nullptr) {
                    LOG("SD already mounted");
                    return true;
                }
                fs_ = (FATFS*) rckid::Heap::malloc(sizeof(FATFS));
                if (f_mount(fs_, "", /* mount immediately */ 1) != FR_OK) {
                    delete fs_;
                    return false;
                }
                return true;
            case Drive::Cartridge:
                if (lfs_.cfg == & lfsCfg_) {
                    LOG("Cartridge already mounted");
                    return true;
                }
                if (lfs_mount(&lfs_, &lfsCfg_) != 0) {
                    TRACE_LITTLEFS("Mounting cartridge failed");
                    lfs_.cfg = nullptr;
                    return false;
                }
                return true;
            default:
                UNREACHABLE;
        }
    }

    void unmount(Drive dr) {
        switch (dr) {
            case Drive::SD:
                f_unmount("");
                rckid::free(fs_);
                break;
            case Drive::Cartridge:
                lfs_unmount(&lfs_);
                lfs_.cfg = nullptr;
                break;
            default:
                UNREACHABLE;
        }
    }

    bool isMounted(Drive dr) {
        switch (dr) {
            case Drive::SD:
                return fs_ != nullptr;
            case Drive::Cartridge:
                return lfs_.cfg != nullptr;
            default:
                UNREACHABLE;
        }
    }

    uint64_t getCapacity(Drive dr) {
        switch (dr) {
            case Drive::SD:
                return static_cast<uint64_t>(fs_->n_fatent - 2) * fs_->csize * 512;
            case Drive::Cartridge:
                return lfsCfg_.block_count * lfsCfg_.block_size;
            default:
                UNREACHABLE;
        }
    }

    uint64_t getFreeCapacity(Drive dr) {
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
    }

    Filesystem getFormat(Drive dr) {
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
    }

    std::string getLabel(Drive dr) {
        switch (dr) {
            case Drive::SD: {
                std::string result{' ', 12};
                f_getlabel("",result.data(), 0);
                return result;
            }
            case Drive::Cartridge:
                return "Cartridge";
            default:
                UNREACHABLE;
        }
    }

    bool exists(char const * path, Drive dr) {
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
    }

    bool isFolder(char const * path, Drive dr) {
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
    }

    bool isFile(char const * path, Drive dr) {
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
    }

    // file read, write and append operations

    FileRead fileRead(char const * path, Drive dr) {
        FileRead result;
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
        return result;
    }

    FileWrite fileWrite(char const * path, Drive dr) {
        FileWrite result;
        switch (dr) {
            case Drive::SD:
                if (f_open(& result.sd_, path, FA_WRITE) == FR_OK)
                    result.drive_ = static_cast<unsigned>(dr);
                break;
            case Drive::Cartridge:
                if (lfs_file_open(& lfs_, & result.cart_, path, LFS_O_WRONLY | LFS_O_CREAT) >= 0)
                    result.drive_ = static_cast<unsigned>(dr);
                break;
            default:
                UNREACHABLE;
        }
        return result;
    }

    FileWrite fileAppend(char const * path, Drive dr) {
        FileWrite result;
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
        return result;
    }

    Folder folderRead(char const * path, Drive dr) {
        Folder result;
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
        return result;
    }



} // namespace rckid::filesystem