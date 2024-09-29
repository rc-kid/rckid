#pragma once

#include <stdlib.h>

#include <avr/sleep.h>
#include <avr/delay.h>
#include <avr/interrupt.h>

#define ARCH_AVR_MEGATINY
#define ARCH_LITTLE_ENDIAN

#include "gpio.h"
#include "cpu.h"
#include "i2c.h"
#include "../common.h"
//#include "../fonts.h"
//#include "../peripherals/color_strip.h"

namespace platform {

}

//#define NO_ISR(...) do { cli(); __VA_ARGS__; sei(); } while (false)
#define NO_ISR(...) __VA_ARGS__
//#define UNREACHABLE while (false) {}
