#pragma once

#include "rckid/rckid.h"

/** SD Card SPI interface
 
    
 */
namespace rckid {

    /** Tells the card to go to idle state (reset) 
     */
    constexpr uint8_t CMD0[] =   { 0x40, 0x00, 0x00, 0x00, 0x00, 0x95 };
    /** Interface condition (voltage & data byte ping-pong to verify connection - 0xaa)
     */
    constexpr uint8_t CMD8[] =   { 0x48, 0x00, 0x00, 0x01, 0xaa, 0x87 };
    /** Send CSD register, returns 16 bytes
     */
    constexpr uint8_t CMD9[] =   { 0x49, 0x00, 0x00, 0x00, 0x00, 0x01 };
    /** Set block length to 512b (only for non SDHC cards)
     */
    constexpr uint8_t CMD16[] =  { 0x50, 0x00, 0x00, 0x02, 0x00, 0x01 };
    /** Application specific command flag. 
     */
    constexpr uint8_t CMD55[] =  { 0x77, 0x00, 0x00, 0x00, 0x00, 0x65 };
    /** Reads the OCR register. 
     */
    constexpr uint8_t CMD58[] =  { 0x7a, 0x00, 0x00, 0x00, 0x00, 0xfd };
    /** App command - send operation condition 
    */
    constexpr uint8_t ACMD41[] = { 0x69, 0x40, 0x00, 0x00, 0x00, 0x77 };

    // response codes
    constexpr uint8_t SD_NO_ERROR = 0;
    constexpr uint8_t SD_IDLE = 1;
    constexpr uint8_t SD_ERASE_RESET = 2;
    constexpr uint8_t SD_ILLEGAL_COMMAND = 4;
    constexpr uint8_t SD_CRC_ERROR = 8;
    constexpr uint8_t SD_ERASE_SEQUENCE_ERROR = 16;
    constexpr uint8_t SD_ADDRESS_ERROR = 32;
    constexpr uint8_t SD_PARAMETER_ERROR = 64;
    constexpr uint8_t SD_VALID = 128;
    constexpr uint8_t SD_BUSY = 255;

    void sdInitialize();

    uint8_t sdSendCommand(uint8_t const (&cmd)[6], uint8_t * response = nullptr, size_t responseSize = 0, unsigned maxDelay = 128);

} // namespace rckid