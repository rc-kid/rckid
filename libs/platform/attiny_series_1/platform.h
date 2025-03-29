#pragma once

#include <stdlib.h>

#include <avr/sleep.h>
#include <avr/delay.h>
#include <avr/interrupt.h>

#define PLATFORM_AVR_MEGATINY
#define PLATFORM_LITTLE_ENDIAN
#define PLATFORM_NOSTDCPP

#undef cli
#undef sei

#include "../definitions.h"

#ifndef ASSERT
#define ASSERT(...)
#endif

namespace std {
    template<typename T>
    T && move(T &) {
        return static_cast<T &&>(T{});
    }
}


class cpu {
public:
    static void delayUs(uint16_t value) {
        _delay_us(value);
    }

    static void delayMs(uint16_t value) {
        while (value-- != 0) {
            // make sure we reset the wdt before cycles for more robustness
            __asm__ __volatile__ ("wdr"::);
            _delay_ms(1);
        }
    }

    static void sleep() {
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
        sleep_cpu();
    }

    static void reset() {
        _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
    }

    static void wdtReset() __attribute__((always_inline)) {
        __asm__ __volatile__ ("wdr"::);
    }

    static void cli() __attribute__((always_inline)) { 
        __asm__ __volatile__ ("cli"::);
    }

    static void sei()  __attribute__((always_inline)){ 
        __asm__ __volatile__ ("sei"::);
    }

    #include "../common/cpu_common.h"


};

class gpio {
public:

    enum class Pin : uint8_t {
        A0 = 0x00, 
        A1, 
        A2, 
        A3, 
        A4, 
        A5, 
        A6, 
        A7, 
        B0 = 0x10, 
        B1, 
        B2, 
        B3, 
        B4, 
        B5, 
        B6, 
        B7, 
        C0 = 0x20,
        C1, 
        C2, 
        C3, 
        C4, 
        C5,
        Unused = 0xff
    };

    static constexpr Pin A0 = Pin::A0;
    static constexpr Pin A1 = Pin::A1;
    static constexpr Pin A2 = Pin::A2;
    static constexpr Pin A3 = Pin::A3;
    static constexpr Pin A4 = Pin::A4;
    static constexpr Pin A5 = Pin::A5;
    static constexpr Pin A6 = Pin::A6;
    static constexpr Pin A7 = Pin::A7;
    static constexpr Pin B0 = Pin::B0;
    static constexpr Pin B1 = Pin::B1;
    static constexpr Pin B2 = Pin::B2;
    static constexpr Pin B3 = Pin::B3;
    static constexpr Pin B4 = Pin::B4;
    static constexpr Pin B5 = Pin::B5;
    static constexpr Pin B6 = Pin::B6;
    static constexpr Pin B7 = Pin::B7;
    static constexpr Pin C0 = Pin::C0;
    static constexpr Pin C1 = Pin::C1;
    static constexpr Pin C2 = Pin::C2;
    static constexpr Pin C3 = Pin::C3;
    static constexpr Pin C4 = Pin::C4;
    static constexpr Pin C5 = Pin::C5;
    static constexpr Pin UNUSED = Pin::Unused;

    #define GPIO_PORT_INDEX(p) (static_cast<uint8_t>(p) >> 4)
    #define GPIO_PIN_INDEX(p) (static_cast<uint8_t>(p) & 0xf)
    #define GPIO_PIN_PORT(p) (GPIO_PORT_INDEX(p) == 0 ? PORTA : (GPIO_PORT_INDEX(p) == 1) ? PORTB : PORTC)
    #define GPIO_PIN_VPORT(p) (GPIO_PORT_INDEX(p) == 0 ? VPORTA : (GPIO_PORT_INDEX(p) == 1) ? VPORTB : VPORTC)
    #define GPIO_PIN_PINCTRL(PIN) *(& GPIO_PIN_PORT(PIN).DIR + 0x10 + GPIO_PIN_INDEX(PIN))

    static void setAsOutput(Pin p) {
        GPIO_PIN_VPORT(p).DIR |= (1 << GPIO_PIN_INDEX(p));
    }

    static void setAsInput(Pin p) {
        GPIO_PIN_PINCTRL(p) &= ~PORT_PULLUPEN_bm;
        GPIO_PIN_VPORT(p).DIR &= ~(1 << GPIO_PIN_INDEX(p));
    }

    static void setAsInputPullup(Pin p) {
        GPIO_PIN_PINCTRL(p) |= PORT_PULLUPEN_bm;
        GPIO_PIN_VPORT(p).DIR &= ~(1 << GPIO_PIN_INDEX(p));
    }

    static void write(Pin p, bool value) {
        if (value) 
            GPIO_PIN_VPORT(p).OUT |= 1 << GPIO_PIN_INDEX(p);
        else
            GPIO_PIN_VPORT(p).OUT &= ~(1 << GPIO_PIN_INDEX(p));
    }

    static bool read(Pin p) {
        return GPIO_PIN_VPORT(p).IN & (1 << GPIO_PIN_INDEX(p));
    }

    static constexpr uint8_t getADC0muxpos(Pin p) {
        switch (p) {
            case Pin::A0:
            case Pin::A1:
            case Pin::A2:
            case Pin::A3:
            case Pin::A4:
            case Pin::A5:
            case Pin::A6:
            case Pin::A7:
                return GPIO_PIN_INDEX(p);
            case Pin::B5:
                return 8;
            case Pin::B6:
                return 9;
            case Pin::B1:
                return 10;
            case Pin::B0:
                return 11;
            default:
                return 0xff;
        }
    }

    static constexpr uint8_t getADC1muxpos(Pin p) {
        switch (p) {
            case Pin::A4:
            case Pin::A5:
            case Pin::A6:
            case Pin::A7:
                return GPIO_PIN_INDEX(p) - 4;
            case Pin::B7:
                return 4;
            case Pin::B6:
                return 5;
            case Pin::C0:
            case Pin::C1:
            case Pin::C2:
            case Pin::C3:
            case Pin::C4:
            case Pin::C5:
                return GPIO_PIN_INDEX(p) + 6;
            default:
                return 0xff;
        }
    }

    #include "../common/gpio_common.h"
    
}; // gpio

class i2c {
public:

    static void disableSlave() {
        TWI0.SCTRLA = 0;
    }

    static void disableMaster() {
        TWI0.MCTRLA = 0;
    }

    static void disable() {
        TWI0.SCTRLA = 0;
        TWI0.MCTRLA = 0;
    }

    static void initializeSlave(uint8_t address) {
        // turn I2C off in case it was running before
        disable();
        //TWI0.MCTRLA = 0;
        // make sure that the pins are not out - HW issue with the chip, will fail otherwise
        if (PORTMUX.CTRLB | PORTMUX_TWI0_bm)
            PORTA.OUTCLR = 0x06; // PA1, PA2
        else
            PORTB.OUTCLR = 0x03; // PB0, PB1
        // set the address and disable general call, disable second address and set no address mask (i.e. only the actual address will be responded to)
        TWI0.SADDR = address << 1;
        TWI0.SADDRMASK = 0;
        // enable the TWI in slave mode, don't enable interrupts as the default implementation operates in polling mode
        TWI0.SCTRLA |= TWI_ENABLE_bm;
        // TWI0.SCTRLA = TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm  | TWI_ENABLE_bm;

        // bus Error Detection circuitry needs Master enabled to work 
        // not sure why we need it
        TWI0.MCTRLA = TWI_ENABLE_bm;   
    }

    static void initializeMaster() {
        // turn I2C off in case it was running before
        disable();
        // TODO disable slave as well? 
        // make sure that the pins are nout out - HW issue with the chip, will fail otherwise
        if (PORTMUX.CTRLB | PORTMUX_TWI0_bm)
            PORTA.OUTCLR = 0x06; // PA1, PA2
        else
            PORTB.OUTCLR = 0x03; // PB0, PB1
        uint32_t baud = ((F_CPU/100000) - (((F_CPU* /* rise time */300)/1000)/1000)/1000 - 10)/2;
        TWI0.MBAUD = (uint8_t)baud;
        TWI0.MCTRLA = TWI_ENABLE_bm;         
        TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
    }

    static bool masterTransmit(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize) {
        auto wait = [](){
            while (!(TWI0.MSTATUS & (TWI_WIF_bm | TWI_RIF_bm))) {}; 
        };
        auto waitIdle = [](){
            while (!(TWI0.MSTATUS & TWI_BUSSTATE_IDLE_gc)) {}; 
        };

        auto busLostOrError = [](){
            return TWI0.MSTATUS & (TWI_BUSERR_bm | TWI_ARBLOST_bm);
        };
        auto stop = [&](){
            TWI0.MCTRLB = TWI_MCMD_STOP_gc;
            waitIdle();
        }; 
        auto start = [&](uint8_t address, bool read) {
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
        };
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
    }

    #include "../common/i2c_common.h"

}; // i2c

class serial {
public:

    static void initializeTx(uint32_t speed = 9600) {
        uint16_t baud = (F_CPU / (16UL * speed)) - 1;
        USART0.BAUD = baud;
        USART0.CTRLB = USART_TXEN_bm; // Enable transmitter
        USART0.CTRLC = USART_CHSIZE_8BIT_gc; // Set frame format: 8 data bits, no parity, 1 stop bit
    }

    static void write(char c) {
        while (!(USART0.STATUS & USART_DREIF_bm));
        USART0.TXDATAL = c;
    }

    static void waitForTx() {
        while (!(USART0.STATUS & USART_DREIF_bm));
    }

}; // serial


// TODO add SPI 
