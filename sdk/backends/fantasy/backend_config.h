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


/** To model the memory management on the actual device we create a continuous array of device's memory size and set the bss end and stack limit pointers to it. We add extra room for the stack protection string which is written just above the heap beginning where on the device there is stack, while on the fantasy console there is nothing.
 */
#define RCKID_MEMORY_INITIALIZATION \
    char fantasyHeap[256 * 1024 + 5]; \
    char & __bss_end__ = *fantasyHeap; \
    char & __StackLimit = *(fantasyHeap + sizeof(fantasyHeap) - 5);
