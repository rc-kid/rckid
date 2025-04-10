#pragma once
/** \page backend_fantasy Fantasy Backend 

    A fantasy console backend that uses raylib for graphics & sound. Should work anywhere raylib does, but does not support the HW features of RCKid (some are emulated to a degree, but mileage may vary in those cases)

    \section Configuration

    The following command line arguments are supported for fantasy backend cartridges (executables):

    - `--sd=PATH` sets path to the SD card ISO image, which will be accessible as FAT fs. 
    - `--flash=PATH` sets path to the flash (cartdirge) ISO image, which will be accessible as LittleFS.


    \section Graphics

    Raylib is used for rendering. For now, focus on native rendering only
*/

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
