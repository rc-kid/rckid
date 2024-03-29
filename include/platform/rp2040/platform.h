#pragma once

#include <cstdint>
#include <hardware/gpio.h>
#include <hardware/clocks.h>
#include <hardware/pio.h>
#include <hardware/i2c.h>
#include <hardware/spi.h>
#include <hardware/vreg.h>
#include <hardware/dma.h>
#include <hardware/uart.h>
#include <pico.h>
#include <pico/binary_info.h>
#include <pico/time.h>
#include <pico/stdlib.h>
#include <pico/rand.h>

#define ARCH_RP2040
#define ARCH_LITTLE_ENDIAN

#include "gpio.h"
#include "cpu.h"
#include "i2c.h"
#include "pio.h"
#include "../common.h"
//#include "../I2CDevice.h"
#include "../fonts.h"
#include "../peripherals/color_strip.h"

