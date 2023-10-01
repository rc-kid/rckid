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

namespace platform {

    class cpu {
    public:
        static void delayUs(unsigned value) {
            std::this_thread::sleep_for(std::chrono::microseconds(value));        
        }

        static void delayMs(unsigned value) {
            std::this_thread::sleep_for(std::chrono::milliseconds(value));        
        }

        //static void sleep() {}

    }; // cpu

    //class wdt {
    //public:
    //    static void enable() {}
    //    static void disable() {}
    //    static void reset() {}
    //}; // wdt

    class gpio {
    public:
        using Pin = unsigned;
        static constexpr Pin UNUSED = 0xffffffff;

        enum class Edge {
            Rising = INT_EDGE_RISING, 
            Falling = INT_EDGE_FALLING, 
            Both = INT_EDGE_BOTH,
        }; 

        static void initialize() {
            wiringPiSetupGpio();
        }

        static void output(Pin pin) {
            pinMode(pin, OUTPUT);
        }

        static void input(Pin pin) {
            pinMode(pin, INPUT);
            pullUpDnControl(pin, PUD_OFF);
        }

        static void inputPullup(Pin pin) {
            pinMode(pin, INPUT);
            pullUpDnControl(pin, PUD_UP);
        }

        static void high(Pin pin) {
            digitalWrite(pin, HIGH);
        }

        static void low(Pin pin) {
            digitalWrite(pin, LOW);
        }

        static bool read(Pin pin) { 
            return digitalRead(pin) == HIGH;
        }

        static void attachInterrupt(Pin pin, Edge edge, void(*handler)()) {
            wiringPiISR(pin, (int) edge, handler);
        } 
    }; // gpio

    // https://stackoverflow.com/questions/75246900/sending-i2c-command-from-c-application
    class i2c {
    public:

        static bool initializeMaster() {
            // TODO set speed here too
            // make sure that a write followed by a read to the same address will use repeated start as opposed to stop-start

            handle_ = open("/dev/i2c-1", O_RDWR);
            return handle_ >= 0;
            // old code with pigpio
            //i2cSwitchCombined(true);
        }

        // static void initializeSlave(uint8_t address_) {}

        static bool transmit(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize) {
            i2c_msg msgs[2];
            memset(msgs, 0, sizeof(msgs));
            unsigned nmsgs = 0;
            if (wsize > 0) {
                msgs[nmsgs].addr = static_cast<uint16_t>(address);
                msgs[nmsgs].buf = const_cast<uint8_t*>(wb);
                msgs[nmsgs].len = static_cast<uint16_t>(wsize);
                ++nmsgs;
            }
            if (rsize > 0) {
                msgs[nmsgs].addr = static_cast<uint16_t>(address);
                msgs[nmsgs].buf = rb;
                msgs[nmsgs].len = static_cast<uint16_t>(rsize);
                msgs[nmsgs].flags = I2C_M_RD;
                ++nmsgs;
            }
            // fake zero writes for checking if the chip exists
            if (nmsgs == 0) {
                msgs[nmsgs].addr = static_cast<uint16_t>(address);
                msgs[nmsgs].len = 0;
                ++nmsgs;
            }
            i2c_rdwr_ioctl_data wrapper = {
                .msgs = msgs,
                .nmsgs = nmsgs};
            return ioctl(handle_, I2C_RDWR, &wrapper) >= 0;
            /* // old code with pigpio
            int h = i2cOpen(1, address, 0);
            if (h < 0)
                return false;
            if (wsize != 0)
                if (i2cWriteDevice(h, (char*)wb, wsize) != 0) {
                    i2cClose(h);
                    return false;
                }
            if (rsize != 0)
                if (i2cReadDevice(h, (char *)rb, rsize) != 0) {
                    i2cClose(h);
                    return false;
                }
            return i2cClose(h) == 0;
            */
        }

        static inline int handle_ = -1; 

    }; // i2c


    /** The default spi implementation via pigpio sets the CS line before and after each transfer which does not play nice with the smaller MCUs such as AVR where multiple sequential transfers are used for a single transaction. Fortunately, the CS pins can be ignored by the pigpio, so they can be controlled by the user, which is what we do here. 
     */
    class spi {
    public:

        using Device = gpio::Pin;

        static bool initialize(unsigned baudrate = 5000000) {
            baudrate_ = baudrate;
            handle_ = open("/dev/spidev1.0", O_RDWR);
            if (handle_ >= 0) {
                close(handle_);
                return true;
            } else {
                return false;
            }
        }

        /** Starts the transmission to given device. 
         */
        static void begin(Device device) {
            handle_ = open("/dev/spidev1.0", O_RDWR);
            int mode = SPI_MODE_0;
            uint8_t bpw = 8;


            ioctl(handle_, SPI_IOC_WR_MODE, & mode);
            ioctl(handle_, SPI_IOC_WR_BITS_PER_WORD, & bpw);
            ioctl(handle_, SPI_IOC_WR_MAX_SPEED_HZ, & baudrate_);

            gpio::output(device);
            gpio::low(device);

            /*
            handle_ = spiOpen(0, baudrate_, SPI_AUX | SPI_RES_CE0 | SPI_RES_CE1 | SPI_RES_CE2);
            gpio::low(device);
            */
        }

        /** Terminates the SPI transmission and pulls the CE high. 
         */
        static void end(Device device) {
            gpio::high(device);
            close(handle_);
            /*
            spiClose(handle_);
            gpio::high(device);
            */
        }

        /** Transfers a single byte.
         */
        static uint8_t transfer(uint8_t value) { 
            uint8_t result = 0;
            transfer(& value, & result, 1);
            return result;
            /*
            uint8_t result;
            spiXfer(handle_, reinterpret_cast<char*>(& value), reinterpret_cast<char*>(& result), 1);
            return result;
            */
        }

        static size_t transfer(uint8_t const * tx, uint8_t * rx, size_t numBytes) { 
            struct spi_ioc_transfer spi;
            memset(&spi, 0, sizeof(spi));
            spi.tx_buf = (unsigned long) tx;
            spi.rx_buf = (unsigned long) rx;
            spi.len = numBytes;
            //spi.delayUsecs = 0;
            spi.speed_hz = baudrate_;
            spi.bits_per_word = 8;
            ioctl(handle_, SPI_IOC_MESSAGE(1), &spi);
            return numBytes;
            /*
            spiXfer(handle_, reinterpret_cast<char*>(const_cast<uint8_t*>(tx)), reinterpret_cast<char*>(rx), numBytes);
            return numBytes;
            */
        }

        static void send(uint8_t const * data, size_t size) {
            transfer(data, nullptr, size);
            //for (size_t i = 0; i < size; ++i)
            //    transfer(data[i]);
            //spiWrite(handle_, reinterpret_cast<char*>(const_cast<uint8_t*>(data)), size);
        }

        static void receive(uint8_t * data, size_t size) {
            transfer(nullptr, data, size);
            //wiringPiSPIDataRW (handle_, data, size);
            //spiRead(handle_, reinterpret_cast<char*>(data), size);
        }


    private:

        static constexpr unsigned SPI_AUX = 1 << 8;
        static constexpr unsigned SPI_RES_CE0 = 1 << 5;
        static constexpr unsigned SPI_RES_CE1 = 1 << 6;
        static constexpr unsigned SPI_RES_CE2 = 1 << 7;

        static inline unsigned baudrate_;

        static inline int handle_ = -1; 
        
    }; // spi

} // namespace platform