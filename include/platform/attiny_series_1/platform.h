#pragma once

#include <stdlib.h>

#include <avr/sleep.h>
#include <avr/delay.h>
#include <avr/interrupt.h>

#define ARCH_AVR_MEGATINY

#include "gpio.h"
#include "cpu.h"
#include "i2c.h"
#include "../I2CDevice.h"
#include "../fonts.h"
#include "../utils.h"

//#define NO_ISR(...) do { cli(); __VA_ARGS__; sei(); } while (false)
#define NO_ISR(...) __VA_ARGS__
#define UNREACHABLE while (false) {}
