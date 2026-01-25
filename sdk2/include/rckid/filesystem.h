#pragma once

#include <platform.h>

#include <rckid/string.h>
#include <rckid/stream.h>

/** Filesystem Access
  
    RCKid SDK provides simplified access to two fileystems available on the device - SD card based FAT filesystem and a LittleFS based filesystem inside the cartridge flash memory. Those are presented as two distinct drives with identical APIs for mounting, unmounting, formatting and file / folder access.

    File reads and writes are supported via streams. 

    TODO discuss what should go to what filesystem conceptually. 

    TODO mention the host filesystem support for fantasy console, once implemented.

    TODO implement the filesystem functions, but API is good to go
 */

namespace rckid::fs {

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

    /** Simple informtion about an entry inside a folder.
     */
    class FolderEntry {
    public:
        String name;
        bool isFolder;
        uint32_t size;
    }; // FolderEntry

    /** Returns whether the given drive is already mounted. 
     */
    bool isMounted(Drive dr = Drive::SD);

    /** Returns the capacity of the selected drive in bytes. 
     
        The drive does not have to be mounted for this function to work. 
     */
    uint64_t getCapacity(Drive dr = Drive::SD);

    /** Mounts the specified drive. 
     
        Returns true if the drive was mounted (i.e. the card is present and its format has been understood, or cartridge has allocated filesystem space and has been formatted correctly), false otherwise (no SD card or corrupted data). 
     */
    bool mount(Drive dr = Drive::SD);

    /** Unmounts previously mounted drive.
     
        No-op if the drive is currently not mounted. 
     */
    void unmount(Drive dr = Drive::SD);

    /** Returns the available free capacity on the drive.
     
        Depending on the underlying filesystem and its size, this can take time and shouldnot be overly trusted as it inly represents the best effort. 
     */
    uint64_t getFreeCapacity(Drive dr = Drive::SD);  


    uint32_t readFolder(String const & path, Drive dr, std::function<void(FolderEntry const &)> callback);

    inline uint32_t readFolder(String const & path,std::function<void(FolderEntry const &)> callback) {
        return readFolder(path, Drive::SD, callback);
    }

    unique_ptr<RandomReadStream> openRead(String const & path, Drive dr = Drive::SD);

    unique_ptr<RandomWriteStream> openWrite(String const & path, Drive dr = Drive::SD);

    unique_ptr<RandomWriteStream> openAppend(String const & path, Drive dr = Drive::SD);
    


} // namespace rckid::fs