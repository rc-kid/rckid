#include <platform.h>
#include <platform/peripherals/nrf24l01.h>

#include <secrets.h>

#include <common/esp8266/bridge.h>

#include <common/radio/radio.h>

#define RADIO_NRF_PIN_CS 2
#define RADIO_NRF_PIN_RXTX 16
#define RADIO_NRF_PIN_IRQ 0

#include <common/radio/nrf.cpp>

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


/*
class Radio {
public:

    static void initialize() {
        spi::initialize();
        LOG("Initializing radio...");
        gpio::setAsInput(RADIO_NRF_PIN_IRQ);
        bool result = radio_.initializeESB("  RK1", "  RK1", 87);
        if (!result)
            LOG("  FAILED");
        char addr[] = {0,0,0,0,0,0};
        radio_.setAddressLength(3);
        radio_.enablePipe2(0, true);
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
        if (gpio::read(RADIO_NRF_PIN_IRQ) == 0) {
            LOG("NRF IRQ detected...");
            radio_.clearDataReadyIrq();
            uint8_t msg[32];
            while (radio_.receive(msg, 32)) {
                LOG("    message: " << Writer::hex{msg, 32});
            }
        }
    }

private:

    static inline platform::NRF24L01 radio_{RADIO_NRF_PIN_CS, RADIO_NRF_PIN_RXTX};

}; // Radio

*/

using namespace rckid;


class RadioController : public radio::Controller {
public:

    static void initialize(radio::DeviceId address) {
        spi::initialize();
        LOG("Initializing radio...");
        //gpio::setAsInput(RADIO_NRF_PIN_IRQ);

        // initialize the radio
        radio::initialize(address);
        radio::enable();
        // and create the singleton 
        new RadioController{};
        LOG(" rx address: " << (uint32_t)address);
        LOG(" should be receiving");
        LOG(" status: " << (uint32_t)(radio::nrf().getStatus().raw));
        char addr[] = {0,0,0,0,0,0};
        radio::nrf().txAddress(addr);
        LOG("  tx addr:" << addr);    
        radio::nrf().rxAddress(addr);
        LOG("  rx addr:" << addr);    
        LOG("  channel: " << (uint32_t)radio::nrf().channel());

    }

protected:

}; // RadioController

void setup() {
    // start serial protocol for debugging
    Serial.begin(115200);
    Bridge::initialize();
    Bridge::connect();

    RadioController::initialize('1');
    LOG("Setup done");
}   

void loop() {
    Bridge::loop();

    // for now do polling for the radio
    radio::loop();
}