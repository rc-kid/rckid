#pragma once

#include "pico/stdlib.h"
#include "hardware/pwm.h"

#include "../config.h"

namespace rckid {

    /** PWM subsystem - times & audio out 
      
     */
    class PWM {
    public:

        /** Initializes the PWM channel 
         */
        static void initialize() {
            gpio_set_function(RP_PIN_PWM_RIGHT, GPIO_FUNC_PWM);
            gpio_set_function(RP_PIN_PWM_LEFT, GPIO_FUNC_PWM);

            // Find out which PWM slice is connected to GPIO 0 (it's slice 4)
            uint slice_num = pwm_gpio_to_slice_num(RP_PIN_PWM_RIGHT);

            // Set period of 1024 cycles (0 to 1023 inclusive)
            pwm_set_wrap(slice_num, 1023);
            // Set channel A output high for one cycle before dropping
            pwm_set_chan_level(slice_num, PWM_CHAN_A, 1);
            // Set initial B output high for three cycles before dropping
            pwm_set_chan_level(slice_num, PWM_CHAN_B, 512);
            // Set the PWM running
            pwm_set_enabled(slice_num, true);
            /// \end::setup_pwm[]

            uint16_t i = 0;
            while (true) {
                pwm_set_chan_level(slice_num, PWM_CHAN_A, i++);
                if (i >= 1023)
                    i = 0;
                sleep_us(1);
            }
              



        }



    }; // rckid::PWM


} // namespace rckid