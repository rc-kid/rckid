#pragma once

#include <platform/peripherals/nrf24l01.h>

namespace rckid::radio {

    extern uint8_t status_;

    platform::NRF24L01 & nrf();

}