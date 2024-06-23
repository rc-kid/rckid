#pragma once

#define ARCH_ESP8266


/** On ESP8266 we use the default Arduino core provided for basic hardware. 
 */
#include "../arduino/platform.h"

#include "utils/writer.h"

inline Writer writeToSerial() {
    return Writer{[](char x) {
        Serial.write(&x, 1);
        //if (x == '\n')
        //    tud_cdc_write_flush();            
    }};
}

#define LOG(...) ::writeToSerial() << __VA_ARGS__ << "\r\n"


/** Minimal circuit

    - D15 pull low 
    - D2 pull high
    - D0 pull high for normal operation, pull low for serial programming
    - CH_PD must be pulled high
    - RST must be pulled high, low to reset 

    Optional 

    - D16 can be used to wake the chip up

 


    Device Pinouts
 
    ESP-12

                    ANTENNA

             RST -           - D1 (TX)
             ADC -           - D3 (RX)
           CH_PD -           - D5 (SCL)
             D16 -           - D4 (SDA)
       (SCK) D14 -           - D0
      (MISO) D12 -           - D2 (onboard LED 470R)
      (MOSI) D13 -           - D15 (SS)
             VCC -           - GND

    ESP-201

             GND -           - 3V3
             GND -  ANTENNA  - 3V3
        (SCL) D5 -           - D4 (SDA)
             ADC -           -
             RST -           -
           CH_PD -           -
             D16 -           -
       (SCK) D14 -           -
      (MISO) D12 -           -
      (MOSI) D13 -           - D2
        (SS) D15 -           - D0
                    | | | |

                 GND D1 D3 3V3
                   (TX) (RX)    

    NOTE: the serial connection should mate RX - TX and TX - RX, so for most USB to serial cables I have, the connection rt colors is:

    - GND = black
    - TX = white
    - RX = green
    - 3V3 = red

    To connect to the serial port for debugging, the following can be used:

        picocom -b 115200 /dev/ttyUSB0

    (make sure to replace the baudrate and serial device according to the project at hand, this line is for by dev server and USB to serial dongle)

 */
