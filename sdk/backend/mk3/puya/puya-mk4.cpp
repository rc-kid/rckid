#include <platform.h>
#include <writer.h>


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