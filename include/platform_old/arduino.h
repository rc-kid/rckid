#pragma once
#include <Arduino.h>
#if (!defined ARCH_AVR_MEGATINY)
#include <Wire.h>
#include <SPI.h>
#endif

#if (defined ARCH_AVR_MEGA) || (defined ARCH_AVR_MEGATINY)
#include <avr/sleep.h>
#include <avr/delay.h>
#endif

/** Missing features of the standard library. 
 
    Extending the std namespace is undefined behavior, but it works...
 */
namespace std {
    template<typename T1, typename T2>
    struct pair {
        T1 first;
        T2 second;

        pair(T1 first, T2 second): first{first}, second{second} {}
    };

    template<typename T1, typename T2>
    pair<T1, T2> make_pair(T1 first, T2 second) {
        return pair<T1, T2>{first, second};
    }

}

template<typename T>
T pow(T base, uint8_t exp) {
    T result = 1;
    while (exp-- != 0) 
        result *= base;
    return result;
}

void * operator new(size_t size, uint8_t * addr) { return (void*) addr; }

namespace platform {

    class cpu {
    public:

        static void delayUs(uint16_t value) {
            _delay_us(value);
        }

        static void delayMs(uint16_t value) {
            _delay_ms(value);
        }

        static void sleep() {
#if (defined ARCH_AVR_MEGATINY)
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);
            sleep_enable();
            sleep_cpu();
#endif
        }

        static void reset() {
#if (defined ARCH_AVR_MEGATINY)
            _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
#endif
        }

    }; // cpu

    class wdt {
    public:
        static void enable() {
#if (defined ARCH_AVR_MEGATINY)
            _PROTECTED_WRITE(WDT.CTRLA,WDT_PERIOD_8KCLK_gc); // no window, 8sec
#endif
        }
        static void disable() {
#if (defined ARCH_AVR_MEGATINY)
            _PROTECTED_WRITE(WDT.CTRLA,0);
#endif

        }
        static void reset() __attribute__((always_inline)) {
#if (defined ARCH_AVR_MEGATINY)
            __asm__ __volatile__ ("wdr"::);
#endif
        }
    }; // wdt


    class gpio {
    public:
        using Pin = int;
        static constexpr Pin UNUSED = -1;

        static void initialize() {}

        static void output(Pin pin) {
            pinMode(pin, OUTPUT);
        }

        static void input(Pin pin) {
            pinMode(pin, INPUT);
        }

        static void inputPullup(Pin pin) {
            pinMode(pin, INPUT_PULLUP);
        }

        static void high(Pin pin) {
            digitalWrite(pin, HIGH);
        }

        static void low(Pin pin) {
            digitalWrite(pin, LOW);
        }

        static bool read(Pin pin) {
            return digitalRead(pin);
        }

        static void write(Pin pin, bool value) {
            value ? gpio::high(pin) : gpio::low(pin);
        }
    }; // gpio

    class i2c {
    public:

        static void initializeMaster() {
#if (defined __AVR_ATmega8__)
            TWBR = static_cast<uint8_t>((F_CPU / 100000 - 16) / 2);
            TWAR = 0; // master mode
            TWDR = 0xFF; // default content = SDA released.
            TWCR = Bits<TWEN,TWIE>::value();         
#elif (defined ARCH_AVR_MEGATINY)
            cli();
            // turn I2C off in case it was running before
            TWI0.MCTRLA = 0;
            TWI0.SCTRLA = 0;
            // make sure that the pins are nout out - HW issue with the chip, will fail otherwise
            if (PORTMUX.CTRLB | PORTMUX_TWI0_bm)
                PORTA.OUTCLR = 0x06; // PA1, PA2
            else
                PORTB.OUTCLR = 0x03; // PB0, PB1
            uint32_t baud = ((F_CPU/400000) - (((F_CPU* /* rise time */300)/1000)/1000)/1000 - 10)/2;
            TWI0.MBAUD = (uint8_t)baud;
            TWI0.MCTRLA = TWI_ENABLE_bm;         
            TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
            sei();
#else 
            Wire.begin();
            Wire.setClock(400000);
#endif
        }

        static void initializeSlave(uint8_t address) {
#if (defined ARCH_AVR_MEGATINY)
            cli();
            // turn I2C off in case it was running before
            TWI0.MCTRLA = 0;
            TWI0.SCTRLA = 0;
            // make sure that the pins are nout out - HW issue with the chip, will fail otherwise
            if (PORTMUX.CTRLB | PORTMUX_TWI0_bm)
                PORTA.OUTCLR = 0x06; // PA1, PA2
            else
                PORTB.OUTCLR = 0x03; // PB0, PB1
            // set the address and disable general call, disable second address and set no address mask (i.e. only the actual address will be responded to)
            TWI0.SADDR = address << 1;
            TWI0.SADDRMASK = 0;
            // enable the TWI in slave mode, enable all interrupts
            TWI0.SCTRLA = TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm  | TWI_ENABLE_bm;
            // bus Error Detection circuitry needs Master enabled to work 
            // not sure why we need it
            TWI0.MCTRLA = TWI_ENABLE_bm;   
            sei();
#endif
        }

        static bool transmit(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize) {
#if (defined __AVR_ATmega8__)
        // TODO won't compile for now
        // update the slave address by shifting it to the right and then adding 1 if we are going to read immediately, i.e. no transmit bytes
            address <<= 1;
            if (txSize == 0)
                address += 1;
        i2c_master_start:
            // send the start condition
            TWCR = Bits<TWEN, TWINT, TWSTA>::value();
            switch (Wait()) {
                case START:
                case REPEATED_START:
                    break;
                default:
                    goto i2c_master_error;
            }
            TWDR = address;
            TWCR = Bits<TWEN,TWINT>::value();
            // determine whether we are sending or receiving 
            switch (Wait()) {
                case SLAVE_WRITE_ACK: {
                    // if slave write has been acknowledged, transmit the bytes we have to send
                    uint8_t const * txBytes = static_cast<uint8_t const *>(tx);
                    while (txSize > 0) {
                        TWDR = *txBytes;
                        ++txBytes;
                        --txSize;
                        TWCR = Bits<TWEN,TWINT>::value();
                        if (Wait() != SLAVE_DATA_WRITE_ACK)
                            goto i2c_master_error;
                    }
                    // done, send the stop condition if there are no bytes to be received
                    if (rxSize == 0)
                        break;
                    // otherwise update slave address to indicate reading and send repeated start
                    slave += 1;
                    goto i2c_master_start;
                }
                case SLAVE_READ_ACK: {
                    uint8_t * rxBytes = static_cast<uint8_t *>(rx);
                    while (rxSize > 0) {
                        if (rxSize == 1) 
                            TWCR = Bits<TWEN, TWINT>::value();
                        else
                            TWCR = Bits<TWEN, TWINT, TWEA>::value();
                        switch (Wait()) {
                            case SLAVE_DATA_READ_ACK:
                            case SLAVE_DATA_READ_NACK:
                                break;
                        default:
                            goto i2c_master_error;
                        }
                        *rxBytes = TWDR;
                        ++rxBytes;
                        --rxSize;
                    }
                    break;
                }
                default:
                    goto i2c_master_error;
            }
            // transmit the stop condition and exit successfully
            TWCR = Bits<TWINT,TWEN,TWSTO>::value();
            return true;
            // an error
        i2c_master_error:
            TWCR = Bits<TWINT,TWEN,TWSTO>::value();
            return false;
#elif (defined ARCH_AVR_MEGATINY)
            if (wsize > 0) {
                if (! start(address, false)) 
                    goto i2c_master_error;
                // we send all we have no matter what 
                for (uint8_t i = 0; i < wsize; ++i) {
                    TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;                    
                    TWI0.MDATA = *(wb++); 
                    wait();
                    if (busLostOrError()) {
                        stop();
                        goto i2c_master_error;
                    }
                }
                if (rsize == 0)
                    stop();
            }
            if (rsize > 0) {
                if (! start(address, true))
                    goto i2c_master_error;
                while (rsize-- > 0) {
                    wait();
                    *(rb++) = TWI0.MDATA;
                    TWI0.MCTRLB = (rsize > 0) ? TWI_MCMD_RECVTRANS_gc : TWI_ACKACT_NACK_gc;
                }
                stop();
            }
            return true;
        i2c_master_error:
            TWI0.MCTRLA = TWI_FLUSH_bm;
            return false;
#else
            if (wsize > 0) {
                Wire.beginTransmission(address);
                Wire.write(wb, wsize);
                Wire.endTransmission(rsize == 0); 
            }
            if (rsize > 0) {
                if (Wire.requestFrom(address, rsize) == rsize) {
                    Wire.readBytes(rb, rsize);
                } else {
                    Wire.flush();
                    return false;
                }
            }
            return true;
#endif
        }


    private:
#if (defined ARCH_AVR_MEGATINY)

        static bool start(uint8_t address, bool read) {
            TWI0.MADDR = (address << 1) | read;
            wait();
            if (busLostOrError()) {
                waitIdle();
                return false;
            }
            if (TWI0.MSTATUS & TWI_RXACK_bm) {
                stop();
                return false;
            }
            return true;
        }

        static void stop() {
            TWI0.MCTRLB = TWI_MCMD_STOP_gc;
            waitIdle();
        }

        static void wait() {
            while (!(TWI0.MSTATUS & (TWI_WIF_bm | TWI_RIF_bm))) {}; 
        }

        static void waitIdle() {
            while (!(TWI0.MSTATUS & TWI_BUSSTATE_IDLE_gc)) {}; 
        }

        static bool busLostOrError() {
            return TWI0.MSTATUS & (TWI_BUSERR_bm | TWI_ARBLOST_bm);
        }

#endif
    }; // i2c

    class spi {
    public:

        using Device = gpio::Pin;

        static void initialize() {
#if (defined ARCH_AVR_MEGATINY)
            // important, first clear the SS pin, then enable the SPI otherwise the code might hang
            SPI0.CTRLB = SPI_SSD_bm;
            SPI0.CTRLA = SPI_MASTER_bm | SPI_ENABLE_bm | SPI_CLK2X_bm;
    #if (defined ARCH_ATTINY_1616) || (defined ARCH_ATTINY_3216)
            gpio::output(16); // SCK
            gpio::input(15); // MISO
            gpio::output(14); // MOSI
    #elif (defined ARCH_ATTINY_1604)
            gpio::output(10); // SCK
            gpio::input(9); // MISO
            gpio::output(8); // MOSI
    #else
            ARCH_NOT_SUPPORTED;
    #endif
#else
            SPI.begin();
#endif
        }

        static void begin(Device device) {
            gpio::low(device);
#if (defined ARCH_AVR_MEGATINY)
            // no need to do anything here since drive the chip's SPI directly
#else
            SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
#endif
        }

        static void end(Device device) {
            gpio::high(device);
#if (defined ARCH_AVR_MEGATINY)
            // no need to do anything here since drive the chip's SPI directly
#else
            SPI.endTransaction();
#endif
        }

        static uint8_t transfer(uint8_t value) {
#if (defined ARCH_AVR_MEGATINY)
            // megatinycore says this speeds up by 10% - have to check that this is really the truth
            //asm volatile("nop");
            SPI0.DATA = value;
            while (! (SPI0.INTFLAGS & SPI_IF_bm));
            return SPI0.DATA;            
#else
            return SPI.transfer(value);
#endif
        }

        static size_t transfer(uint8_t const * tx, uint8_t * rx, size_t numBytes) { 
            for (size_t i = 0; i < numBytes; ++i)
                *(rx++) = transfer(*(tx++));
            return numBytes;
        }

        static void send(uint8_t const * data, size_t numBytes) {
            for (size_t i = 0; i < numBytes; ++i)
                transfer(*(data++));
        }

        static void receive(uint8_t * data, size_t numBytes) {
            for (size_t i = 0; i < numBytes; ++i)
                *(data++) = transfer(0);
        }

    }; // spi
} // namespace platform