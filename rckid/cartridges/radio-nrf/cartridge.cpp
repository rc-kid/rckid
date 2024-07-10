#include "common/config.h"
#include "common/radio/radio.h"
#include "rckid/rckid.h"

namespace rckid::cartridge {

    void initialize() {
        spi::initialize(RADIO_NRF_PIN_MISO, RADIO_NRF_PIN_MOSI, RADIO_NRF_PIN_SCK);
        //spi::setSpeed(10000000);
        // the line below does not seme to be needed anymore
        //spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
        radio::initialize(radio::BroadcastId);
    }

    void yield() {
        radio::loop();
    }

}