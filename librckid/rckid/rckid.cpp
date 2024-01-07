#include "bsp/board.h"
#include "tusb_config.h"
#include "tusb.h"

#include "common/config.h"
#include "rckid.h"
#include "ST7789.h"
#include "audio.h"
#include "app.h"

extern char __StackLimit, __bss_end__;


namespace rckid {

    void initialize() {
        // FIXME for reasons I do not completely understand, the board init must be before the other calls, or the device hangs? 
        board_init();
        // initialize the display
        ST7789::initialize();
        // initialize the I2C bus
        i2c_init(i2c0, RP_I2C_BAUDRATE); 
        gpio_set_function(RP_PIN_SDA, GPIO_FUNC_I2C);
        gpio_set_function(RP_PIN_SCL, GPIO_FUNC_I2C);
        // Make the I2C pins available to picotool
        bi_decl(bi_2pins_with_func(RP_PIN_SDA, RP_PIN_SCL, GPIO_FUNC_I2C));  
        // TODO detect and initialize the standard peripherals
        // TODO serial if necessary
        tud_init(BOARD_TUD_RHPORT);    
        LOG("RCKid initialized");
    }

    void yield() {
        tud_task();
        Audio::processEvents();
    }

    void start(BaseApp && app) {
        int errorCode = setjmp(Device::fatalError_);
        if (errorCode != 0) 
            Device::BSOD(errorCode);
        app.run();
    }

    Writer writeToUSBSerial() {
        return Writer{[](char x) {
            tud_cdc_write(& x, 1);
            if (x == '\n')
                tud_cdc_write_flush();            
        }};
    }



    size_t freeHeap() {
        size_t heapSize = &__StackLimit  - &__bss_end__;    
        return heapSize - mallinfo().uordblks;
    }


    void powerOff() {
        /// TODO: make sure sd and other things are done first, only then poweroff
        Device::sendCommand(cmd::PowerOff{}); 
    }

    void cpuOverclock(unsigned hz, bool overvolt) {
        if (overvolt) {
            vreg_set_voltage(VREG_VOLTAGE_1_20);
            sleep_ms(10);
        } else {
            // TODO non-overvolt                
        }
        Device::clockSpeed_ = hz;
        set_sys_clock_khz(hz / 1000, true);
    }


    // TODO super dumb nanosecond-like delay. Should be changed to take into account the actual cpu clock speed etc
    void sleep_ns(uint32_t ns) {
        //ns = ns * 2;
        while (ns >= 8) 
          ns -= 8;
    }

    // 4.4ms for system currently

    void Device::tick() {
        lastState_ = state_;
        // query the AVR for the status bytes, first set the address
        i2c0->hw->enable = 0;
        i2c0->hw->tar = AVR_I2C_ADDRESS;
        i2c0->hw->enable = 1;
        // add commands for getting the blocks
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_STOP_BITS; // 1 for read, stop
        while (i2c0->hw->rxflr != 6) 
            yield();
        uint8_t * raw = reinterpret_cast<uint8_t*>(&state_);
        for (int i = 0; i < 6; ++i)
            *(raw++) = i2c0->hw->data_cmd;
        // i2c_read_blocking(i2c0, AVR_I2C_ADDRESS, (uint8_t *)& state_, sizeof(State), false);
    }

    void Device::BSOD(int code) {
        // reset the display
        ST7789::reset();
        ST7789::fill(Color::Blue());
        while(true) {}
    }

} // namespace rckid
