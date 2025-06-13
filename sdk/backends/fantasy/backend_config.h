#pragma once
/** \page backend_fantasy Fantasy Backend 

    A fantasy console backend that uses raylib for graphics & sound. Should work anywhere raylib does, but does not support the HW features of RCKid (some are emulated to a degree, but mileage may vary in those cases)

    \section Configuration

    The following command line arguments are supported for fantasy backend cartridges (executables):

    - `--sd=PATH` sets path to the SD card ISO image, which will be accessible as FAT fs. 
    - `--flash=PATH` sets path to the flash (cartdirge) ISO image, which will be accessible as LittleFS.

    It is useful to be able to access the sd and cartridge iso images from the host system. For the sd, which uses exfat, do the following (compatible with Ubuntu 24.02 LTS in WSL2 mode):

    - `sudo losetup -Pf sd.iso`
    - `sudo mount.exfat-fuse /dev/loop0p1 /mnt/rckid/sd`

    Note that on WSL we must use the fuse driver as exfat is not supported by the kernel. To detach, run:ADJ_ESTERROR
    - `sudo umount /mnt/rckid/sd`
    - `sudo losetup -d /dev/loop0`

    \section Graphics

    Raylib is used for rendering. For now, focus on native rendering only
*/

#define RCKID_ENABLE_STACK_PROTECTION 1
#define RCKID_STACK_LIMIT_SIZE 4000
#define RCKID_MEMORY_SIZE (256 * 1024)

/** Enables host filesystem where the FatFS and LittleFS drivers are bypassed and std::filesystem is used directly to access the sd and cartridge folders on the host machine. This is useful for general fantasy console work as the filesystem drivers do not have to be engaged and both rckid and the host machine can access the sd and cartridge files with ease. 
 */
#define RCKID_ENABLE_HOST_FILESYSTEM 1


/** Display width & height for the UI. 
 */
#define RCKID_DISPLAY_WIDTH 320 
#define RCKID_DISPLAY_HEIGHT 240

#define RCKID_RUMBLER_DEFAULT_STRENGTH 1
#define RCKID_RUMBLER_OK_STRENGTH 1
#define RCKID_RUMBLER_OK_TIME_ON 1
#define RCKID_RUMBLER_OK_TIME_OFF 1
#define RCKID_RUMBLER_OK_CYCLES 1
#define RCKID_RUMBLER_FAIL_STRENGTH 1
#define RCKID_RUMBLER_FAIL_TIME_ON 1
#define RCKID_RUMBLER_FAIL_TIME_OFF 1
#define RCKID_RUMBLER_FAIL_CYCLES 1
#define RCKID_RUMBLER_NUDGE_STRENGTH 1
#define RCKID_RUMBLER_NUDGE_TIME_ON 1
#define RCKID_RUMBLER_NUDGE_TIME_OFF 1
#define RCKID_RUMBLER_NUDGE_CYCLES 1
