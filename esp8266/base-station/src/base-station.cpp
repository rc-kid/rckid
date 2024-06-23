#include "platform.h"

#include "../secrets.h"

#include "nrf24l01.h"

#include "bridge.h"


/** RCKid Base Station 
 
    The base station is a shared WiFi to NRF24L01p bridge. 
 
    # Pinout

    The pinout below is given for ESP-201, but care is taken that ESP-12E can be used equally well. 

                 GND -           - 3V3
                 GND -  ANTENNA  - 3V3
            (SCL) D5 -           - D4 (SDA)
                 ADC -           -
                 RST -           -
               CH_PD -           -
      (NRF-RXTX) D16 -           -
       (NRF-SCK) D14 -           -
      (NRF-MISO) D12 -           -
      (NRF-MOSI) D13 -           - D2 (NRF-CS)
                 D15 -           - D0 (NRF-IRQ)
                        | | | |

                     GND D1 D3 3V3
                       (TX) (RX)    
    
    TODO: How can the bridge URL connection control the radio? 
    
 */



#define PIN_NRF_CS 2
#define PIN_NRF_RXTX 16
#define PIN_NRF_IRQ 0
class Radio {
public:

    static void initialize() {
        spi::initialize();
        LOG("Initializing radio...");
        gpio::setAsInput(PIN_NRF_IRQ);
        bool result = radio_.initializeESB("BSKID", "RCKID", 56);
        if (!result)
            LOG("  FAILED");
        char addr[] = {0,0,0,0,0,0};
        radio_.txAddress(addr);
        LOG("  tx addr:" << addr);    
        radio_.rxAddress(addr);
        LOG("  rx addr:" << addr);    
        LOG("  channel: " << (uint32_t)radio_.channel());
        LOG("  entering standby mode");
        radio_.standby();
        LOG("  enabling receiver");
        radio_.enableReceiver();
        LOG("  DONE");
    }

    static void loop() {
        if (gpio::read(PIN_NRF_IRQ) == 0) {
            LOG("NRF IRQ detected...");
            radio_.clearDataReadyIrq();
            uint8_t msg[32];
            while (radio_.receive(msg, 32)) {
                LOG("    message: " << Writer::hex{msg, 32});
            }
        }
    }

private:

    static inline platform::NRF24L01 radio_{PIN_NRF_CS, PIN_NRF_RXTX};

}; // Radio




void setup() {
    // start serial protocol for debugging
    Serial.begin(115200);
    Bridge::initialize();
    Bridge::connect();

    Radio::initialize();
}   

void loop() {
    Bridge::loop();
    Radio::loop();
}