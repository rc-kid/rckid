#pragma once
#include <cstdint>
#include <cstring>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <wiringPiSPI.h>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <errno.h>

#define ARCH_RPI
#define ARCH_LINUX
#define ARCH_LITTLE_ENDIAN

#include "gpio.h"
#include "cpu.h"
#include "i2c.h"
#include "../utils.h"
#include "../I2CDevice.h"
#include "../fonts.h"

