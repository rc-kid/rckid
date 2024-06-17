
#include <platform.h>

#include "common/config.h"

using namespace platform;


int main() {
    static_assert(AVR_PIN_PWM_RUMBLER == A3); //TCB1 WO
    gpio::outputFloat(AVR_PIN_PWM_RUMBLER);
    TCB1.CTRLA = 0;
    TCB1.CTRLB = 0; 
    TCB1.CCMPL = 255;
    TCB1.CCMPH = 0; 
    gpio::outputLow(AVR_PIN_PWM_RUMBLER);
    TCB1.CCMPH = 128;
    TCB1.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
    TCB1.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm | TCB_RUNSTDBY_bm;
    while(true) {};
}

