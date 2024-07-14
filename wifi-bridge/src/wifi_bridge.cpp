#include <ESP8266WiFi.h>

#include <platform.h>

#include "secrets.h"
#include "bridge.h"

/** Disable wifi at startup. 
 
    From: https://github.com/esp8266/Arduino/issues/6642#issuecomment-578462867
    and:  https://github.com/esp8266/Arduino/tree/master/libraries/esp8266/examples/LowPowerDemo

    TL;DR; DO as much as possible as soon as possible and start setup() with a delay as the delay is really the thing that makes esp enter the sleep mode.
 */

RF_PRE_INIT() {
    system_phy_set_powerup_option(2);  // shut the RFCAL at boot
    wifi_set_opmode_current(NULL_MODE);  // set Wi-Fi working mode to unconfigured, don't save to flash
    wifi_fpm_set_sleep_type(MODEM_SLEEP_T);  // set the sleep type to modem sleep
}

void preinit() {
    wifi_fpm_open();
    wifi_fpm_do_sleep(0xffffffff);
}

/** UART Radio connection
 */
namespace rckid::radio {

    uint8_t buffer[32];
    uint8_t wIndex = 0;

    DeviceId id() { return 1; }
    void initialize(DeviceId) { }
    void enable(bool silent) { }
    void disable() { }

    /** Transmitting a message is simple, just do serial write. Pad with spaces so that we always send 32 bytes.
     */
    void transmit(DeviceId target, uint8_t const * msg, size_t length) {
        Serial.write(msg, length);
        while (length++ < 32)
            Serial.write(' ');
    }

    /** While in the loop, check the serial line for bytes up to a message length and once we have it  */
    void loop() {
        while (Serial.available() > 0) {
            buffer[wIndex++] = static_cast<uint8_t>(Serial.read());
            if (wIndex == 32) {
                Controller::instance_->onMessageReceived(buffer);
                wIndex = 0;
            }
        }
    }
}

/** RCKid WiFi Bridge
 
    For now, do very very little. 
 */

void setup() {
    Serial.begin(74880); // start at the same speed as the starup messages for now
    LOG("Application code ready");
    Bridge::initialize();
    Bridge::connect();
}   

void loop() {
    Bridge::loop();
    rckid::radio::loop();
}


