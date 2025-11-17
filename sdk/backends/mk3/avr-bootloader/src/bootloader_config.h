#pragma once

/** Include the common config for AVR I2C address and IRQ pin. 
 */
#include "common/config.h"

#define BOOTLOADER_SIZE 0x200

namespace bootloader {

    static constexpr uint8_t BOOTLOADER_MODE = 7;

    static constexpr uint8_t CMD_RESERVED = 0x00;
    static constexpr uint8_t CMD_RESET = 0x01;
    static constexpr uint8_t CMD_INFO = 0x02;
    static constexpr uint8_t CMD_WRITE_BUFFER = 0x03;
    static constexpr uint8_t CMD_WRITE_PAGE = 0x04;
    static constexpr uint8_t CMD_SET_ADDRESS = 0x05;

    struct State {
        uint8_t status; 
        uint8_t deviceId0;
        uint8_t deviceId1;
        uint8_t deviceId2;
        uint8_t fuses[11];
        uint8_t mclkCtrlA;
        uint8_t mclkCtrlB;
        uint8_t mclkLock;
        uint8_t mclkStatus;
        uint16_t address;
        uint16_t nvmAddress;
        uint8_t lastError;
        //uint16_t pgmmemstart;
        //uint16_t pagesize;
    } __attribute__((packed));

    static_assert(sizeof(State) == 24, "Invalid state size.");

}
