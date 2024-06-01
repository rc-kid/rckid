#pragma once

#include "rckid/rckid.h"

namespace rckid {

    /** Provides direct access to the SD card and manages between the device and USB access to it. 
     */
    class SD {
    public:

        static bool ready() { return capacity_ > 0; }

        static bool usbMscReady() { return usbMscReady_; }

        static uint32_t numBlocks() { return capacity_ * 2; }

        /** Returns the capacity of the inserted SD card in kilobytes, i.e. a maximum of 4TB is theoretically possible, but such large cards likely do not support the SPI interface anyways. 
         */
        static size_t capacity() { return capacity_; }

        static bool enableUsbMsc(bool value) {
            usbMscReady_ = value;
            return true;
        }

        static uint32_t numMscReads() { return numMscReads_; }
        static uint32_t numMscWrites() { return numMscWrites_; }

    //private:

        /** Initializes the SD card and determines its capacity.
         
            NOTE the function is blocking and will actually take milliseconds (tens of) to complete due to the SD card initialization process. 
         */
        static bool initialize();
        
        /** Reads given 512 bytes block of data. 
         
            NOTE the function is blocking. 
         */
        static bool readBlock(size_t num, uint8_t * buffer);

        /** Writes given 523 bytes block of data. 

            NOTE the function is blocking.  
         */
        static bool writeBlock(size_t num, uint8_t const * buffer);

        static uint8_t sendCommand(uint8_t const (&cmd)[6], uint8_t * response = nullptr, size_t responseSize = 0, unsigned maxDelay = 128);


        // capacity of the SD card in kilobytes, i.e. up to 4TB is technically supported
        static inline size_t capacity_ = 0;

        // usb storage configuration
        static inline bool usbMscReady_ = false;

        static inline uint32_t numMscReads_ = 0;
        static inline uint32_t numMscWrites_ = 0;



        // response codes
        static constexpr uint8_t NO_ERROR = 0;
        static constexpr uint8_t IDLE = 1;
        static constexpr uint8_t ERASE_RESET = 2;
        static constexpr uint8_t ILLEGAL_COMMAND = 4;
        static constexpr uint8_t CRC_ERROR = 8;
        static constexpr uint8_t ERASE_SEQUENCE_ERROR = 16;
        static constexpr uint8_t ADDRESS_ERROR = 32;
        static constexpr uint8_t PARAMETER_ERROR = 64;
        static constexpr uint8_t VALID = 128;
        static constexpr uint8_t BUSY = 255;

        /** Tells the card to go to idle state (reset) 
         */
        static constexpr uint8_t CMD0[] =   { 0x40, 0x00, 0x00, 0x00, 0x00, 0x95 };
        /** Interface condition (voltage & data byte ping-pong to verify connection - 0xaa)
         */
        static constexpr uint8_t CMD8[] =   { 0x48, 0x00, 0x00, 0x01, 0xaa, 0x87 };
        /** Send CSD register, returns 16 bytes
         */
        static constexpr uint8_t CMD9[] =   { 0x49, 0x00, 0x00, 0x00, 0x00, 0x01 };
        /** Set block length to 512b (only for non SDHC cards)
         */
        static constexpr uint8_t CMD16[] =  { 0x50, 0x00, 0x00, 0x02, 0x00, 0x01 };
        /** Application specific command flag. 
         */
        static constexpr uint8_t CMD55[] =  { 0x77, 0x00, 0x00, 0x00, 0x00, 0x65 };
        /** Reads the OCR register. 
         */
        static constexpr uint8_t CMD58[] =  { 0x7a, 0x00, 0x00, 0x00, 0x00, 0xfd };
        /** App command - send operation condition 
        */
        static constexpr uint8_t ACMD41[] = { 0x69, 0x40, 0x00, 0x00, 0x00, 0x77 };

    }; // rckid::SD


} // namespace rckid