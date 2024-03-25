#pragma once

namespace gpio {

    using Pin = uint16_t;

    constexpr Pin UNUSED = 0xffff;

    inline void initialize() {
        //stdio_init_all();
    }

    inline void setAsOutput(Pin pin) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
    }

    inline void setAsInput(Pin pin) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
    }

    inline void setAsInputPullup(Pin pin) {
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    inline void write(Pin pin, bool value) { gpio_put(pin, value); }

    inline bool read(Pin pin) { return gpio_get(pin); }
}; // gpio

