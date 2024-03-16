#pragma once

#include <avr/sleep.h>
#include <avr/delay.h>
#include <avr/interrupt.h>

#include "gpio.h"
#include "cpu.h"
#include "i2c.h"

//#define NO_ISR(...) do { cli(); __VA_ARGS__; sei(); } while (false)
#define NO_ISR(...) __VA_ARGS__
#define UNREACHABLE while (false) {}
