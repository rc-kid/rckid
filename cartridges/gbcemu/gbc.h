#pragma once

#include <cstdint>

/** GameBoy Color Emulator.

 */
class GBC {
public:

    GBC():
        af_{*reinterpret_cast<uint16_t*>(& a_)},
        bc_{*reinterpret_cast<uint16_t*>(& b_)},
        de_{*reinterpret_cast<uint16_t*>(& d_)},
        hl_{*reinterpret_cast<uint16_t*>(& h_)} {
    }

private:

    /** \name CPU Registers and state
     */
    //@{

    class Flags {

    private:
        uint8_t raw_;
    } __attribute__((packed));

    uint8_t a_;
    Flags f_;
    uint8_t b_;
    uint8_t c_;
    uint8_t d_;
    uint8_t e_;
    uint8_t h_;
    uint8_t l_;
    uint16_t sp_;
    uint16_t pc_;

    uint16_t & af_;
    uint16_t & bc_;
    uint16_t & de_;
    uint16_t & hl_;

    //@}



    void loop();



}; // GBC