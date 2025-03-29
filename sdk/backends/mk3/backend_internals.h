#pragma once


/** Memory limits are created automatically by the linker script on the backend so we just have to extern the symbols here. 
 */
#define RCKID_MEMORY_INITIALIZATION \
    extern char __bss_end__; \
    extern char __StackLimit;




#define RCKID_AVR_I2C_ADDRESS 0x43
#define RCKID_PMIC_I2C_ADDRESS 0x6b
#define RCKID_ACCEL_I2C_ADDRESS 0x19
#define RCKID_ALS_I2C_ADDRESS 0x29
#define RCKID_AUDIO_CODEC_ADDRESS 0x1a
#define RCKID_FM_RADIO_ADDRESS 0x60



#define RCKID_AVR_SERIAL_SPEED 115200

