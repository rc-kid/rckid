#include <avr/io.h>

#include "bootloader_config.h"

/**
                   -- VDD             GND --
                   -- (00) PA4   PA3 (16) -- 
                   -- (01) PA5   PA2 (15) -- SCL (I2C)
                   -- (02) PA6   PA1 (14) -- SDA (I2C)
                   -- (03) PA7   PA0 (17) -- UPDI
                   -- (04) PB5   PC3 (13) -- BTN_HOME
                   -- (05) PB4   PC2 (12) -- 
                   -- (06) PB3   PC1 (11) -- 
                   -- (07) PB2   PC0 (10) -- AVR_IRQ
                   -- (08) PB1   PB0 (09) -- BACKLIGHT

*/

#define I2C_DATA_MASK (TWI_DIF_bm | TWI_DIR_bm) 
#define I2C_DATA_TX (TWI_DIF_bm | TWI_DIR_bm)
#define I2C_DATA_RX (TWI_DIF_bm)
#define I2C_START_MASK (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
#define I2C_START_TX (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
#define I2C_START_RX (TWI_APIF_bm | TWI_AP_bm)
#define I2C_STOP_MASK (TWI_APIF_bm | TWI_DIR_bm)
#define I2C_STOP_TX (TWI_APIF_bm | TWI_DIR_bm)
#define I2C_STOP_RX (TWI_APIF_bm)

/* Define application pointer type */
typedef void (*const app_t)(void);

using namespace bootloader;

__attribute__((OS_main)) __attribute__((constructor))
void boot() {
    /* Initialize system for C support */
    asm volatile("clr r1");
    volatile State state;
    register uint8_t * address;
    register uint8_t command;
    // enable pull-up on C3 (BTN_HOME)
    PORTC.PIN3CTRL |= PORT_PULLUPEN_bm;
    // ensure that when AVR_IRQ is switched to output mode, the pin is pulled low
    VPORTC.OUT &= ~ (1 << 0); 
    // enable watchdog (enabling it this early means the app starts with watchdog enabled so if it misbehaves we end up here)
    while (WDT.STATUS & WDT_SYNCBUSY_bm); // required busy wait
    _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_1KCLK_gc);        
    // if BTN_HOME is pressed, the bootloader will be started a keepalive mode (no communication with RPi is necessary to keep it alive since wdt is reset every loop iteration)
    register bool keepalive = ! (VPORTC.IN & PIN3_bm);
    // only enter the bootloader if PC0 (AVR_IRQ) is pulled low
    if (keepalive || (VPORTC.IN & PIN0_bm) == 0) {
        // enable the display backlight when entering the bootloader for some output (like say, eventually an OTA:) Baclight is connected to PB0 and is active high
        VPORTB.DIR |= (1 << 0);
        VPORTB.OUT |= (1 << 0);
        // initialize the I2C in slave mode w/o interrupts, first switch to the alternate pins
        PORTMUX.CTRLB |= PORTMUX_TWI0_bm;
        // turn I2C off in case it was running before
        TWI0.MCTRLA = 0;
        TWI0.SCTRLA = 0;
        // make sure that the pins are not out - HW issue with the chip, will fail otherwise
        PORTA.OUTCLR = 0x06; // PA1, PA2
        // set the address and disable general call, disable second address and set no address mask (i.e. only the actual address will be responded to)
        TWI0.SADDR = AVR_I2C_ADDRESS << 1;
        TWI0.SADDRMASK = 0;
        // enable the TWI in slave mode, enable all interrupts
        TWI0.SCTRLA = TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm  | TWI_ENABLE_bm;
        // bus Error Detection circuitry needs Master enabled to work 
        TWI0.MCTRLA = TWI_ENABLE_bm;  
        // initialize the info structure so that we can return the chip info
        state.status = 7; // Bootloader
        state.deviceId0 = SIGROW.DEVICEID0;
        state.deviceId1 = SIGROW.DEVICEID1;
        state.deviceId2 = SIGROW.DEVICEID2;
        for (uint8_t i = 0; i < 11; ++i)
            state.fuses[i] = ((uint8_t*)(&FUSE))[i];
        state.mclkCtrlA = CLKCTRL.MCLKCTRLA;
        state.mclkCtrlB = CLKCTRL.MCLKCTRLB;
        state.mclkLock = CLKCTRL.MCLKLOCK;  
        state.mclkStatus = CLKCTRL.MCLKSTATUS;
        //state.pgmmemstart = MAPPED_PROGMEM_START;
        //state.pagesize = MAPPED_PROGMEM_PAGE_SIZE;
        // finally start the main loop for the I2C commands 
        while (true) {
            uint8_t status = TWI0.SSTATUS;
            // sending data to accepting master is on our fastpath as is checked first, the next byte in the buffer is sent, wrapping the index on 256 bytes 
            if ((status & I2C_DATA_MASK) == I2C_DATA_TX) {
                TWI0.SDATA = *(address++);
                TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
            } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
                if (command == CMD_RESERVED) {
                    command = TWI0.SDATA;
                    switch (command) {
                        case CMD_INFO:
                            state.nvmAddress = NVMCTRL.ADDR;
                            state.address = address;
                            address = (uint8_t*)(& state);
                            break;
                        case CMD_SET_ADDRESS:
                            address = (uint8_t*)(&state.address);
                            break;
                        default:
                            break;
                    }
                } else {
                    *(address++) = TWI0.SDATA;
                }
                TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            // master requests slave to write data, simply say we are ready as we always send the buffer
            } else if ((status & I2C_START_MASK) == I2C_START_TX) {
                TWI0.SCTRLB = TWI_ACKACT_ACK_gc + TWI_SCMD_RESPONSE_gc;
            // master requests to write data itself, first the command code
            // pull the AVR_IRQ low to signal we are busy with the command by enabling output on PC0
            } else if ((status & I2C_START_MASK) == I2C_START_RX) {
                command = CMD_RESERVED; // command will be filled in 
                TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
                VPORTC.DIR |= (1 << 0); 
            // sending finished, there is nothing to do 
            } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
                TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            // receiving finished, if we are in command mode, process the command, otherwise there is nothing to be done 
            } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
                TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
                switch (command) {
                    // Reset the AVR - first signal the command is processed, then reset the chip
                    case CMD_RESET:
                        VPORTB.DIR &= ~(1 << 0);
                        _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
                    case CMD_WRITE_PAGE:
                        _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
                        while (NVMCTRL.STATUS & (NVMCTRL_FBUSY_bm | NVMCTRL_EEBUSY_bm));
                        state.lastError = NVMCTRL.STATUS;                        
                        break;
                    case CMD_SET_ADDRESS:
                        address = state.address;
                        break;
                }
                // each command receive resets the watchdog
                __asm__ __volatile__ ("wdr\n");
                // switch AVR_IRQ (PC0) to input again indicating we are done processing the command
                VPORTC.DIR &= ~(1 << 0);
            // nothing to do, or error - not sure what to do, perhaps reset the bootloader? 
            } else {
                // TODO
            }
            // if the keepalive flag is set, reset watchdog at each cycle
            if (keepalive)
                __asm__ __volatile__ ("wdr\n");
        }
    }
    // enable the boot lock so that app can't override the bootloader and then start the app
    NVMCTRL.CTRLB = NVMCTRL_BOOTLOCK_bm;
    // start the application
    app_t app = (app_t)(BOOTLOADER_SIZE / sizeof(app_t));
    app();    
}