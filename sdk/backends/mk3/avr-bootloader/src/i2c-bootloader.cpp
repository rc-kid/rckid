#include <platform.h>
#include <platform/peripherals/neopixel.h>
#include <avr/io.h>



#include "../../backend_internals.h"


namespace rckid::bootloader {

    /** Reserved value. Indicates readiness to obtain command.
     */
    constexpr uint8_t CMD_RESERVED     = 0x00;
    constexpr uint8_t CMD_INFO         = 0x11;
    /** Resets the chip.
     */
    constexpr uint8_t CMD_RESET        = 0x22;
    /** Sets the address from which the next transaction read or write be operating. As the flas is memory mapped on the chip, this allows the bootloader to read/write anywhere in the memory */
    constexpr uint8_t CMD_SET_ADDRESS  = 0x33;
    constexpr uint8_t CMD_WRITE        = 0x44;
    constexpr uint8_t CMD_WRITE_MEM    = 0x55;
    
    /** State returned by the bootloader. 
     
        The state mimics the AVRState used in normal operation so that same code on RP2350 can be used to get both. However, the bottloader bit (second byte MSB) which AVRState will never set will be always on in the bootloader state. 
     */
    PACKED(class State {
    public:
        uint8_t status;
        uint8_t const bootloaderFlag = 0xff;
        uint8_t deviceId[3];
        uint8_t fuses[11];
        uint8_t mclkCtrlA;
        uint8_t mclkCtrlB;
        uint8_t mclkLock;
        uint8_t mclkStatus;
        uint16_t address;
        uint16_t nvmAddress;
    });
}

static constexpr uint8_t I2C_DATA_MASK  = (TWI_DIF_bm | TWI_DIR_bm);
static constexpr uint8_t I2C_DATA_TX    = (TWI_DIF_bm | TWI_DIR_bm);
static constexpr uint8_t I2C_DATA_RX    = (TWI_DIF_bm);
static constexpr uint8_t I2C_START_MASK = (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm);
static constexpr uint8_t I2C_START_TX   = (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm);
static constexpr uint8_t I2C_START_RX   = (TWI_APIF_bm | TWI_AP_bm);
static constexpr uint8_t I2C_STOP_MASK  = (TWI_APIF_bm | TWI_DIR_bm);
static constexpr uint8_t I2C_STOP_TX    = (TWI_APIF_bm | TWI_DIR_bm);
static constexpr uint8_t I2C_STOP_RX    = (TWI_APIF_bm);

/* Define application pointer type */
typedef void (*const app_t)(void);

using namespace rckid::bootloader;

int main() {
    // first thing - read the first two bytes of EEPROM, if they are 0xff, it means we should enter bootloader mode, otherwise start the application
    uint16_t version = *(uint16_t*)(0x1400);
    if (version != 0xffff) {
        // enable the boot lock so that app can't override the bootloader and then start the app
        NVMCTRL.CTRLB = NVMCTRL_BOOTLOCK_bm;
        // start the application
        app_t app = (app_t)(cpu::bootloaderSize() / sizeof(app_t));
        app();    
    }
    // enable 2 second watchdog so that the second tick resets it always with enough time to spare
    while (WDT.STATUS & WDT_SYNCBUSY_bm); // required busy wait
        _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_2KCLK_gc);      
    // set CLK_PER prescaler to 2, i.e. 10Mhz, which is the maximum the chip supports at voltages as low as 3.0V
    CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm;
    // enable 5V power to LEDs and show select & start white backlight to indicate bootloader mode
    gpio::outputHigh(AVR_PIN_5V_EN);
    gpio::setAsOutput(AVR_PIN_RGB);
    cpu::delayMs(50);
    platform::NeopixelStrip<NUM_RGB_LEDS> rgb{AVR_PIN_RGB};
    rgb[4] = platform::Color::White().withBrightness(64);
    rgb[5] = platform::Color::White().withBrightness(64);
    rgb.update(true);
    // set brightness to maximum so that whatever RP2350 is showing is visible
    gpio::outputHigh(AVR_PIN_PWM_BACKLIGHT);
    // turn on IOVDD so that the RP2350 is powered and set the AVR INT as 
    gpio::outputFloat(AVR_PIN_AVR_INT);
    gpio::outputHigh(AVR_PIN_IOVDD_EN);
    // initialize the I2C in slave mode
    i2c::initializeSlave(RCKID_AVR_I2C_ADDRESS);
    TWI0.SCTRLA |= TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm;

    // the state we will be sending back, fill it with device information
    volatile State state;
    state.status = 0;
    state.deviceId[0] = SIGROW.DEVICEID0;
    state.deviceId[1] = SIGROW.DEVICEID1;
    state.deviceId[2] = SIGROW.DEVICEID2;
    for (uint8_t i = 0; i < 11; ++i)
        state.fuses[i] = ((uint8_t*)(&FUSE))[i];
    state.mclkCtrlA = CLKCTRL.MCLKCTRLA;
    state.mclkCtrlB = CLKCTRL.MCLKCTRLB;
    state.mclkLock = CLKCTRL.MCLKLOCK;  
    state.mclkStatus = CLKCTRL.MCLKSTATUS;

    // address for reading and writing data during communication and the command received. By default, there is no command and the address is set to the state itself so that the initial read is identical
    register uint8_t * address = (uint8_t *)(& state);
    register uint8_t command = CMD_RESERVED;
    // do I2C communication in loop. When instructed to send data, simply send data from the current address
    while (true) {
        // reset the watchdog timer
        cpu::wdtReset();
        uint8_t status = TWI0.SSTATUS;
        // sending data to accepting master is on our fastpath as is checked first, the next byte in the buffer is sent, wrapping the index on 256 bytes 
        if ((status & I2C_DATA_MASK) == I2C_DATA_TX) {
            TWI0.SDATA = *(address++);
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
        // when byte is received, this can either be a command (if no command has been set yet, or is simply a byte to be written at the current address). For particular commands that set or read specific data, we also adjust the reading address accordingly. CMD_INFO stores the current address into the state address, hile the CMD_SET_ADDRESS sets the address to the address field. 
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
        // master requests slave to write data, simply say we are ready as we always send data the address register points to
        } else if ((status & I2C_START_MASK) == I2C_START_TX) {
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc + TWI_SCMD_RESPONSE_gc;
        // master requests to write data itself, first the command code
        // pull the AVR_IRQ low to signal we are busy with the command by enabling output on PC0
        } else if ((status & I2C_START_MASK) == I2C_START_RX) {
            command = CMD_RESERVED; // command will be filled in 
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            gpio::outputLow(AVR_PIN_AVR_INT);
        // sending finished, there is nothing to do 
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        // receiving finished, if we are in command mode, process the command, otherwise there is nothing to be done 
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            switch (command) {
                // reset the AVR - no need to signal anything, just reset as the AVR reset will also cut power to RB2350 briefly
                case CMD_RESET:
                    _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
                // writes the last written data to memory to the non-volatile storage (flash or EEPROM)
                case CMD_WRITE_MEM:
                    _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
                    while (NVMCTRL.STATUS & (NVMCTRL_FBUSY_bm | NVMCTRL_EEBUSY_bm));
                    state.status = NVMCTRL.STATUS;                        
                    break;
                // if the command was to set the address for buffer communication, set the address to the addr register
                case CMD_SET_ADDRESS:
                    address = state.address;
                    break;
            }
            // signal the command has been processed
            gpio::outputFloat(AVR_PIN_AVR_INT);
        // nothing to do, or error - not sure what to do, perhaps reset the bootloader? 
        } else {
            // TODO
        }
    }
}



#ifdef FOOBAR

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


#endif