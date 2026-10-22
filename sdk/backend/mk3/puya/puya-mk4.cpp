#include <platform.h>
#include <writer.h>

/** PY32F030 as IO MCU
 
    In mkIV we are switching from the ATTiny3217 to PY32F030 as the IO MCU. This is mostly to save costs as the ATTiny3217 is even more expensive than the RP2350 itself. 

 */


inline Writer debugWrite() {
    return Writer{serial::write};
}


int main() {
    cpu::initialize();
    serial::initializeTx(115200, gpio::Pin::PF3);
    debugWrite() << "Hello from Puya!" << 67 << "\r\n";
    while (true) {

    }
}