#include "common/config.h"
#include "common/radio/radio.h"
#include "rckid/rckid.h"

namespace rckid::cartridge {

    /** Initialize the UART connection to the ESP8266 inside the cartridge. 
     
        The UART uses pins 16 (TX) and 17 (RX), which correspond to UART0 instance. 
     */
    void initialize() {
#if (defined ARCH_RP2040)
        uart_init(RADIO_WIFI_UART, RADIO_WIFI_BAUDRATE);
        gpio_set_function(RADIO_WIFI_PIN_TX, GPIO_FUNC_UART);
        gpio_set_function(RADIO_WIFI_PIN_RX, GPIO_FUNC_UART);
        gpio::setAsInput(RADIO_WIFI_PIN_RESET);
        gpio::setAsInput(RADIO_WIFI_PIN_FLASH);

        // TODO turn the chip off (put to sleep)
#else 
        // in mock mode, start the curl thread
        radio::initialize(1);

#endif
    }

    void yield() {
        radio::loop();
    }

} // rckid::cartridge
