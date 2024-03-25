#pragma once

#include <cstdint>
#include <hardware/gpio.h>
#include <hardware/clocks.h>
#include <hardware/pio.h>
#include <hardware/i2c.h>
#include <hardware/spi.h>
#include <pico/binary_info.h>

#define ARCH_RP2040
#define ARCH_LITTLE_ENDIAN

#include "gpio.h"
#include "cpu.h"
#include "i2c.h"
#include "../utils.h"
#include "../I2CDevice.h"
#include "../fonts.h"
#include "../peripherals/color_strip.h"

