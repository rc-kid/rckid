#pragma once

#include "rckid.h"

namespace rckid::filesystem {

    /** Possible filesystem formats understood by the SDK. 
     
        exFAT is preferred.
     */
    enum class Filesystem {
        FAT16,
        FAT32,
        exFAT, 
        Unrecognized
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

    /** Returns the capacity of the mounted SD card in bytes. 
     */
    uint64_t getCapacity();

    /** Returns the available free capacity on the device.
     
        Depending on the underlying filesystem and its size, this can take time. 
     */
    uint64_t getFreeCapacity();  

    /** Returns the filesystem used on the SD card. 

        Not very important from the user's perspective as all the formats are abstracted away.  
     */
    Filesystem getFormat();

    /** Returns the SD Card label. 
     */
    std::string getLabel(); 


} // namespace rckid::filesystem