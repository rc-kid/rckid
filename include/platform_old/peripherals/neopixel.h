#pragma once


/** Library for working with neopixels. Comes with the megatinycore and does not require any additional arduino setup. 

    NOTE The library does not work well with 10MHz speed, 8MHz (or likely 16MHz) should be used instead. On 10MHz when turned completely off, the neopixels still gave faint light. The timing is actually really werid. Requires 8MHz clock debug info, *but* 10MHz actual clock.

 */

#include "platform/color.h"

namespace platform { 

    template<uint16_t SIZE>
    class NeopixelStrip : public ColorStrip<SIZE> {
        using ColorStrip<SIZE>::colors_;
        using ColorStrip<SIZE>::changed_;
    public:

        NeopixelStrip(gpio::Pin pin):
            pin_{pin},
            port_{portOutputRegister(digitalPinToPort(pin_))} {
            //pinMode(pin,OUTPUT);
        }

        /** Updates the neopixels. 
         
            Note that the neopixels need a 50us gap after the updte to latch and one must be given (by e.g. never calling the update method too often).
        */
        void update(bool force = false) {
            // don't do anything if we don't need to
            if (!changed_ && !force)
                return;
    #if (defined ARCH_AVR_MEGATINY)
            uint8_t pinMask = digitalPinToBitMask(pin_);
            volatile uint16_t
                i   = SIZE * 3; // Loop counter
            volatile uint8_t
            *ptr = reinterpret_cast<uint8_t*>(colors_),   // Pointer to next byte
                b   = *ptr++,   // Current byte value
                hi,             // PORT w/output bit set high
                lo;             // PORT w/output bit set low
            cli();
            // 8 MHz(ish) AVRxt ---------------------------------------------------------
            #if (F_CPU >= 7400000UL) && (F_CPU <= 9500000UL)

                volatile uint8_t n1, n2 = 0;  // First, next bits out

                // We need to be able to write to the port register in one clock
                // to meet timing constraints here.

                // 10 instruction clocks per bit: HHxxxxxLLL
                // OUT instructions:              ^ ^    ^   (T=0,2,7)

                hi   = *port_ |  pinMask;
                lo   = *port_ & ~pinMask;
                n1 = lo;
                if (b & 0x80) n1 = hi;

                // Dirty trick: RJMPs proceeding to the next instruction are used
                // to delay two clock cycles in one instruction word (rather than
                // using two NOPs).  This was necessary in order to squeeze the
                // loop down to exactly 64 words -- the maximum possible for a
                // relative branch.

                asm volatile(
                "headD:"                   "\n\t" // Clk  Pseudocode
                // Bit 7:
                "st   %a[port], %[hi]"    "\n\t" // 1    PORT = hi
                "mov  %[n2]   , %[lo]"    "\n\t" // 1    n2   = lo
                "st   %a[port], %[n1]"    "\n\t" // 1    PORT = n1
                "rjmp .+0"                "\n\t" // 2    nop nop
                "sbrc %[byte] , 6"        "\n\t" // 1-2  if (b & 0x40)
                "mov %[n2]   , %[hi]"    "\n\t" // 0-1   n2 = hi
                "st   %a[port], %[lo]"    "\n\t" // 1    PORT = lo
                "rjmp .+0"                "\n\t" // 2    nop nop
                // Bit 6:
                "st   %a[port], %[hi]"    "\n\t" // 1    PORT = hi
                "mov  %[n1]   , %[lo]"    "\n\t" // 1    n1   = lo
                "st   %a[port], %[n2]"    "\n\t" // 1    PORT = n2
                "rjmp .+0"                "\n\t" // 2    nop nop
                "sbrc %[byte] , 5"        "\n\t" // 1-2  if (b & 0x20)
                "mov %[n1]   , %[hi]"    "\n\t" // 0-1   n1 = hi
                "st   %a[port], %[lo]"    "\n\t" // 1    PORT = lo
                "rjmp .+0"                "\n\t" // 2    nop nop
                // Bit 5:
                "st   %a[port], %[hi]"    "\n\t" // 1    PORT = hi
                "mov  %[n2]   , %[lo]"    "\n\t" // 1    n2   = lo
                "st   %a[port], %[n1]"    "\n\t" // 1    PORT = n1
                "rjmp .+0"                "\n\t" // 2    nop nop
                "sbrc %[byte] , 4"        "\n\t" // 1-2  if (b & 0x10)
                "mov %[n2]   , %[hi]"    "\n\t" // 0-1   n2 = hi
                "st   %a[port], %[lo]"    "\n\t" // 1    PORT = lo
                "rjmp .+0"                "\n\t" // 2    nop nop
                // Bit 4:
                "st   %a[port], %[hi]"    "\n\t" // 1    PORT = hi
                "mov  %[n1]   , %[lo]"    "\n\t" // 1    n1   = lo
                "st   %a[port], %[n2]"    "\n\t" // 1    PORT = n2
                "rjmp .+0"                "\n\t" // 2    nop nop
                "sbrc %[byte] , 3"        "\n\t" // 1-2  if (b & 0x08)
                "mov %[n1]   , %[hi]"    "\n\t" // 0-1   n1 = hi
                "st   %a[port], %[lo]"    "\n\t" // 1    PORT = lo
                "rjmp .+0"                "\n\t" // 2    nop nop
                // Bit 3:
                "st   %a[port], %[hi]"    "\n\t" // 1    PORT = hi
                "mov  %[n2]   , %[lo]"    "\n\t" // 1    n2   = lo
                "st   %a[port], %[n1]"    "\n\t" // 1    PORT = n1
                "rjmp .+0"                "\n\t" // 2    nop nop
                "sbrc %[byte] , 2"        "\n\t" // 1-2  if (b & 0x04)
                "mov %[n2]   , %[hi]"    "\n\t" // 0-1   n2 = hi
                "st   %a[port], %[lo]"    "\n\t" // 1    PORT = lo
                "rjmp .+0"                "\n\t" // 2    nop nop
                // Bit 2:
                "st   %a[port], %[hi]"    "\n\t" // 1    PORT = hi
                "mov  %[n1]   , %[lo]"    "\n\t" // 1    n1   = lo
                "st   %a[port], %[n2]"    "\n\t" // 1    PORT = n2
                "rjmp .+0"                "\n\t" // 2    nop nop
                "sbrc %[byte] , 1"        "\n\t" // 1-2  if (b & 0x02)
                "mov %[n1]   , %[hi]"    "\n\t" // 0-1   n1 = hi
                "st   %a[port], %[lo]"    "\n\t" // 1    PORT = lo
                "rjmp .+0"                "\n\t" // 2    nop nop
                // Bit 1:
                "st   %a[port], %[hi]"    "\n\t" // 1    PORT = hi
                "mov  %[n2]   , %[lo]"    "\n\t" // 1    n2   = lo
                "st   %a[port], %[n1]"    "\n\t" // 1    PORT = n1
                "rjmp .+0"                "\n\t" // 2    nop nop
                "sbrc %[byte] , 0"        "\n\t" // 1-2  if (b & 0x01)
                "mov %[n2]   , %[hi]"    "\n\t" // 0-1   n2 = hi
                "st   %a[port], %[lo]"    "\n\t" // 1    PORT = lo
                "sbiw %[count], 1"        "\n\t" // 2    i-- (don't act on Z flag yet)
                // Bit 0:
                "st   %a[port], %[hi]"    "\n\t" // 1    PORT = hi
                "mov  %[n1]   , %[lo]"    "\n\t" // 1    n1   = lo
                "st   %a[port], %[n2]"    "\n\t" // 1    PORT = n2
                "ld   %[byte] , %a[ptr]+" "\n\t" // 2    b = *ptr++
                "sbrc %[byte] , 7"        "\n\t" // 1-2  if (b & 0x80)
                "mov %[n1]   , %[hi]"    "\n\t" // 0-1   n1 = hi
                "st   %a[port], %[lo]"    "\n\t" // 1    PORT = lo
                "brne headD"              "\n"   // 2    while(i) (Z flag set above)
                : [ptr]   "+e" (ptr),
                [byte]  "+r" (b),
                [n1]    "+r" (n1),
                [n2]    "+r" (n2),
                [count] "+w" (i)
                : [port]   "e" (port_),
                [hi]     "r" (hi),
                [lo]     "r" (lo));
            #elif (F_CPU >= 9500000UL) && (F_CPU <= 11100000UL)
                /*
                volatile uint8_t n1, n2 = 0;  // First, next bits out
                */
                // 14 instruction clocks per bit: HHHHxxxxLLLLL
                // ST instructions:               ^   ^   ^   (T=0,4,7)
                volatile uint8_t next;

                hi   = *port_ |  pinMask;
                lo   = *port_ & ~pinMask;
                next = lo;
                if (b & 0x80) {
                next = hi;
                }

                // Don't "optimize" the OUT calls into the bitTime subroutine;
                // we're exploiting the RCALL and RET as 3- and 4-cycle NOPs!
                asm volatile(
                "headD:"                   "\n\t" //        (T =  0)
                "st   %a[port], %[hi]"    "\n\t" //        (T =  1)
                "rcall bitTimeD"          "\n\t" // Bit 7  (T = 14)
                "st   %a[port], %[hi]"    "\n\t"
                "rcall bitTimeD"          "\n\t" // Bit 6
                "st   %a[port], %[hi]"    "\n\t"
                "rcall bitTimeD"          "\n\t" // Bit 5
                "st   %a[port], %[hi]"    "\n\t"
                "rcall bitTimeD"          "\n\t" // Bit 4
                "st   %a[port], %[hi]"    "\n\t"
                "rcall bitTimeD"          "\n\t" // Bit 3
                "st   %a[port], %[hi]"    "\n\t"
                "rcall bitTimeD"          "\n\t" // Bit 2
                "st   %a[port], %[hi]"    "\n\t"
                "rcall bitTimeD"          "\n\t" // Bit 1
                // Bit 0:
                "st   %a[port], %[hi]"    "\n\t" // 1    PORT = hi    (T =  1)
                //"rjmp .+0"                "\n\t" // 2    nop nop      (T =  3)
                "nop" "\n\t" // the above seemed to be too long a pause
                "ld   %[byte] , %a[ptr]+" "\n\t" // 2    b = *ptr++   (T =  5)
                "st   %a[port], %[next]"  "\n\t" // 1    PORT = next  (T =  6)
                "mov  %[next] , %[lo]"    "\n\t" // 1    next = lo    (T =  7)
                "sbrc %[byte] , 7"        "\n\t" // 1-2  if (b & 0x80) (T =  8)
                "mov %[next] , %[hi]"    "\n\t" // 0-1    next = hi  (T =  9)
                "st   %a[port], %[lo]"    "\n\t" // 1    PORT = lo    (T = 10)
                "sbiw %[count], 1"        "\n\t" // 2    i--          (T = 12)
                "brne headD"              "\n\t" // 2    if (i != 0) -> (next byte) (T = 14)
                "rjmp doneD"             "\n\t"
                "bitTimeD:"               "\n\t" //      nop nop nop     (T =  4)
                "st   %a[port], %[next]" "\n\t" // 1    PORT = next     (T =  5)
                "mov  %[next], %[lo]"    "\n\t" // 1    next = lo       (T =  6)
                "rol  %[byte]"           "\n\t" // 1    b <<= 1         (T =  7)
                "sbrc %[byte], 7"        "\n\t" // 1-2  if (b & 0x80)    (T =  8)
                    "mov %[next], %[hi]"    "\n\t" // 0-1   next = hi      (T =  9)
                "st   %a[port], %[lo]"   "\n\t" // 1    PORT = lo       (T = 10)
                "ret"                    "\n\t" // 4    nop nop nop nop (T = 14)
                "doneD:"                 "\n"
                : [ptr]   "+e" (ptr),
                [byte]  "+r" (b),
                [next]  "+r" (next),
                [count] "+w" (i)
                : [port]   "e" (port_),
                [hi]     "r" (hi),
                [lo]     "r" (lo));
            #else
                #error "AVR Frequency not supported!
            #endif

            #else
                #error "Platform not supported!"
            #endif
            sei();
        }
        
    private:

        uint8_t pin_;
        volatile uint8_t * port_;

    }; 

} // namespace platform
