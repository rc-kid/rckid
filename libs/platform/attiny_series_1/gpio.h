#pragma once

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
};

constexpr Pin A0 = Pin::A0;
constexpr Pin A1 = Pin::A1;
constexpr Pin A2 = Pin::A2;
constexpr Pin A3 = Pin::A3;
constexpr Pin A4 = Pin::A4;
constexpr Pin A5 = Pin::A5;
constexpr Pin A6 = Pin::A6;
constexpr Pin A7 = Pin::A7;
constexpr Pin B0 = Pin::B0;
constexpr Pin B1 = Pin::B1;
constexpr Pin B2 = Pin::B2;
constexpr Pin B3 = Pin::B3;
constexpr Pin B4 = Pin::B4;
constexpr Pin B5 = Pin::B5;
constexpr Pin B6 = Pin::B6;
constexpr Pin B7 = Pin::B7;
constexpr Pin C0 = Pin::C0;
constexpr Pin C1 = Pin::C1;
constexpr Pin C2 = Pin::C2;
constexpr Pin C3 = Pin::C3;
constexpr Pin C4 = Pin::C4;
constexpr Pin C5 = Pin::C5;

#define GPIO_PORT_INDEX(p) (static_cast<uint8_t>(p) >> 4)
#define GPIO_PIN_INDEX(p) (static_cast<uint8_t>(p) & 0xf)
#define GPIO_PIN_PORT(p) (GPIO_PORT_INDEX(p) == 0 ? PORTA : (GPIO_PORT_INDEX(p) == 1) ? PORTB : PORTC)
#define GPIO_PIN_VPORT(p) (GPIO_PORT_INDEX(p) == 0 ? VPORTA : (GPIO_PORT_INDEX(p) == 1) ? VPORTB : VPORTC)
#define GPIO_PIN_PINCTRL(PIN) *(& GPIO_PIN_PORT(PIN).DIR + 0x10 + GPIO_PIN_INDEX(PIN))

namespace gpio {

    constexpr uint8_t GPIO_PIN_PULLUP = 0b00001000;

    /** Initializes the GPIO. 
     
        Does nothing for ATTiny, kept here for compatibility. 
    */
    void initialize() {}

    /** Sets given pin output pin. 
     */
    void setAsOutput(Pin p) {
        GPIO_PIN_VPORT(p).DIR |= (1 << GPIO_PIN_INDEX(p));
    }

    void setAsInput(Pin p) {
        GPIO_PIN_PINCTRL(p) &= GPIO_PIN_PULLUP;
        GPIO_PIN_VPORT(p).DIR &= ~(1 << GPIO_PIN_INDEX(p));
    }

    void setAsInputPullup(Pin p) {
        GPIO_PIN_PINCTRL(p) |= GPIO_PIN_PULLUP;
        GPIO_PIN_VPORT(p).DIR &= ~(1 << GPIO_PIN_INDEX(p));
    }

    void write(Pin p, bool value) {
        if (value) 
            GPIO_PIN_VPORT(p).OUT |= 1 << GPIO_PIN_INDEX(p);
        else
            GPIO_PIN_VPORT(p).OUT &= ~(1 << GPIO_PIN_INDEX(p));
    }

    bool read(Pin p) {
        return GPIO_PIN_VPORT(p).IN & (1 << GPIO_PIN_INDEX(p));
    }

    constexpr uint8_t getADC0muxpos(Pin p) {
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

    constexpr uint8_t getADC1muxpos(Pin p) {
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


} // namespace gpio
