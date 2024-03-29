#pragma once

namespace i2c {

    inline void initializeMaster(unsigned sda, unsigned scl, unsigned baudrate = 400000) {
        i2c_init(i2c0, baudrate);
        gpio_set_function(sda, GPIO_FUNC_I2C);
        gpio_set_function(scl, GPIO_FUNC_I2C);
        // Make the I2C pins available to picotool
        bi_decl(bi_2pins_with_func(sda, scl, GPIO_FUNC_I2C));    
    }

    //static void initializeSlave(uint8_t address_) {}

    inline bool masterTransmit(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize) {
        if (wsize != 0)
            i2c_write_blocking(i2c0, address, wb, wsize, rsize != 0);
        if (rsize != 0)
            i2c_read_blocking(i2c0, address, rb, rsize, false);
        return true;
    }

}; // i2c

