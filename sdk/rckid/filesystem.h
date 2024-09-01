#pragma once

#include "rckid.h"

namespace rckid::filesystem {

    /** Possible filesystem formats understood by the SDK. 
     
        exFAT is preferred.
     */
    enum class Filesystem {
        FAT16,
        FAT32,
        exFAT
    };

    /** Formats the SD card to given filesystem. 
     
        Requires the card to be not mounted. 
     */
    bool format(Filesystem fs);

    /** Mounts the SD card. 
     
        Returns true if the card was mounted (i.e. the card is present and its format has been understood), false otherwise (no SD card or corrupted data). 
     */
    bool mount();

    /** Unmounts previously mounted SD card. 
     
        No-op if the card is currently not mounted. 
     */
    void unmount();

} // namespace rckid::filesystem