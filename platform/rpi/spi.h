#pragma once

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

        gpio::setAsOutput(device);
        gpio::write(device, false);

        /*
        handle_ = spiOpen(0, baudrate_, SPI_AUX | SPI_RES_CE0 | SPI_RES_CE1 | SPI_RES_CE2);
        gpio::low(device);
        */
    }

    /** Terminates the SPI transmission and pulls the CE high. 
     */
    static void end(Device device) {
        gpio::write(device, true);
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
