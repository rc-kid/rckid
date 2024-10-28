#pragma once
#include "FatFS/ff.h"

#include "rckid.h"
#include "utils/stream.h"

namespace rckid::filesystem {

    /** Possible filesystem formats understood by the SDK. 
     */
    enum class Filesystem {
        FAT16,
        FAT32,
        exFAT, 
        Unrecognized
    };

    class FileReadStream : public RandomReadStream {
    public:

        static FileReadStream open(char const * filename) {
            FileReadStream result;
            f_open(& result.f_, filename, FA_READ);
            return result;
        }

        ~FileReadStream() {
            f_close(& f_);
        }

        bool good() const  { return f_.obj.fs != nullptr; }

        uint32_t size() const override { return f_size(& f_); }

        uint32_t seek(uint32_t position) override {
            f_lseek(& f_, position);
            return f_.fptr;
        }

        uint32_t read(uint8_t * buffer, uint32_t bufferSize) override {
            UINT bytesRead = 0;
            f_read(& f_, buffer, bufferSize, & bytesRead);
            return bytesRead;
        }

    private:
        FIL f_;
    };

    class FileWriteStream : public WriteStream {
    public:

        static FileWriteStream open(char const * filename, bool append = false) {
            FileWriteStream result;
            f_open(& result.f_, filename, FA_WRITE | append ? 0 : FA_OPEN_APPEND);
            return result;
        }

        ~FileWriteStream() {
            f_close(& f_);
        }


        bool good() const  { return f_.obj.fs != nullptr; }

        uint32_t write(uint8_t const * buffer, uint32_t bufferSize) override {
            UINT bytesWritten = 0; 
            f_write(& f_, buffer, bufferSize, & bytesWritten);
            return bytesWritten;
        }

    private:
        FIL f_;
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

    /** Returns true if the given path exists (i.e. either directory, or file).
     */
    bool exists(char const * path);

    /** Returns true if  the given path is a valid file. 
     */
    bool isDir(char const * path);

    /** Returns true if the given path is a valid directory. 
     */
    bool isFile(char const * path); 

} // namespace rckid::filesystem