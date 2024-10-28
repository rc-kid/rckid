#include <FatFS/ff.h>
#include <FatFS/diskio.h>


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
}

namespace rckid::filesystem {

    namespace {
        FATFS * fs_ = nullptr;
    }

    bool format(Filesystem fs) {
        MKFS_PARM opts;
        switch (fs) {
            case Filesystem::FAT16:
                opts.fmt = FM_FAT;
                break;
            case Filesystem::FAT32:
                opts.fmt = FM_FAT32;
                break;
            case Filesystem::exFAT:
                opts.fmt = FM_EXFAT;
                break;
            default:
                ASSERT(fs != Filesystem::Unrecognized);
                UNREACHABLE;
        }
        opts.n_fat = 0; 
        opts.align = 0;
        opts.n_root = 0;
        opts.au_size = 0;
        BYTE work[FF_MAX_SS];
        return f_mkfs("", & opts, work, FF_MAX_SS) == FR_OK;
    }

    bool mount() {
        if (fs_ != nullptr) {
            LOG("Filesystem already mounted");
            return true;
        }
        fs_ = (FATFS*) rckid::malloc(sizeof(FATFS));
        return f_mount(fs_, "", /* mount immediately */ 1) == FR_OK;
    }

    void unmount() {
        f_unmount("");
        rckid::free(fs_);
    }

    uint64_t getCapacity() {
        return static_cast<uint64_t>(fs_->n_fatent - 2) * fs_->csize * 512;
    }

    uint64_t getFreeCapacity() {
        DWORD n;
        FATFS * fs;
        f_getfree("", & n, &fs);
        return static_cast<uint64_t>(n) * fs_->csize * 512;
    }

    Filesystem getFormat() {
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
    }

    std::string getLabel() {
        std::string result{' ', 12};
        f_getlabel("",result.data(), 0);
        return result;
    }

    bool exists(char const * path) {
        FILINFO f;
        if (f_stat(path, & f) != FR_OK)
            return false;
        return f.fname[0] != 0;        
    }

    bool isDir(char const * path) {
        FILINFO f;
        if (f_stat(path, & f) != FR_OK)
            return false;
        return f.fname[0] != 0 && (f.fattrib & AM_DIR);        
    }

    bool isFile(char const * path) {
        FILINFO f;
        if (f_stat(path, & f) != FR_OK)
            return false;
        return f.fname[0] != 0 && ! (f.fattrib & AM_DIR);        
    }

} // namespace rckid::filesystem