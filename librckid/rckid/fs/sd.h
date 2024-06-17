#pragma once

#include "FatFS/ff.h"
#include "rckid/rckid.h"

namespace rckid {

    // forward declaration for friendship purposes
    void initialize();

    /** Provides direct access to the SD card and manages between the device and USB access to it. 
     */
    class SD {
    public:

        /** Status of the attached SD card. When not present, no card was detected. When ready, the card can be accessed using the filesystem module and when in USB mode the filesystem module is detached while the card can be directly accessed via the USB as USB-MSC, i.e. data can be transferred between RCKid and a computer.
        */
        enum class Status {
            NotPresent,
            Unrecognized,
            Ready,
            USB,
        };

        /** Returns the current statusof the SD card. 
         */
        static Status status();

        /** Shorthand for checking that the SD card is ready. 
         */
        static bool ready();

        /** Returns the number of blocks available at the SD card. Block is always 512 bytes long. 
         */
        static uint32_t numBlocks();

        /** Enables, or disables the USB MSC features. 
          
            When enabled, unmounts the filesystem, which will be remounted then the USB MSC mode is left. 
         */
        static void enableUSBMsc(bool value); 

        enum class Format {
            Unrecognized,
            FAT12, 
            FAT16, 
            FAT32, 
            EXFAT,
        };

        /** Returns the capacity of the inserted SD card in bytes. 
         */
        static uint64_t getCapacity();

        /** Returns the available free capacity on the device.
         
            Depending on the underlying filesystem and its size, this can take time. 
         */
        static uint64_t getFreeCapacity();  

        /** Returns the filesystem used on the SD card. 

            Not very important from the user's perspective as all the formats are abstracted away.  
         */
        static Format getFormatKind();

        /** Returns the SD Card label. 
         */
        static std::string getLabel(); 

        /** File that supports basic reading & writing. 
         
            TODO also add async read support with callbacks. 
         */
        class File {
        public:
            static File openRead(std::string const & path);
            static File openWrite(std::string const & path);

            File(File const &) = delete;

            File(File && from):
                f_{from.f_} {
                f_.obj.fs = nullptr; // invalidate the from file object
            }

            ~File() { f_close(& f_); }

            bool good() const { return f_.obj.fs != nullptr; }

            /** Reads data from the file to the provided buffer. 
             
                At most numBytes will be read at once. Returns the actual number of bytes read. If this number is 0, the end of file has been reached, or the request failed.
            */
            uint32_t read(uint8_t * buffer, uint32_t numBytes);

        private:

            File() = default;

            FIL f_;
        }; // SD::File


        /** Simple folder elements iterator. 
         */
        class Folder {
        public:

            static Folder open(std::string const & path);

            Folder(Folder const &) = delete;
            
            Folder(Folder && from):
                d_{from.d_} {
                d_.obj.fs = nullptr; // invalidate;
            }

            ~Folder() { f_closedir(& d_); }

            bool good() const { return d_.obj.fs != nullptr; }

        private:
            Folder() = default;
            DIR d_;

        }; // SD::Folder

    private:

        friend void initialize();

        /** Initializes the SD card, determines its capacity and if found, mounts it to the FatFS module. 
         
            NOTE the function is blocking and will actually take milliseconds (tens of) to complete due to the SD card initialization process. 
         */
        static bool initialize();

    }; // rckid::SD


} // namespace rckid