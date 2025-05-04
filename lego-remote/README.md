# LEGO Remote Controller for RCKid

> This is verbatim copy of the code I created a while ago for mkI. It should be restructured and updated to the mk3 code. Consists of the firmware for the arduino, the remote.h protocol specification and the lego-remote.h device configuration. 

## Pairing

The control brick is very dumb and does not have any internal state (apart from its name) that would survive power cycle, all is managed by the remote. The handshake looks like this:

                              Controller | Remote
                         --------------- | --------------
                                         | L: (listen for new devices - optional) 
    B: broadcast its name on set channel | 
                                         | P1: send pairing info on set channel
                                         | (own name & channel, wait for ACK)
                                         |
            switch channel & wait for P2 | switch to channel, send P2, wait for ACK
                                         |
         (timeout 1 second, return to B) | (timeout 1 second, return to L, or P1)

While pairing the controller flashes red lights. When paired, blue is flashed. Upon pairing the remote uploads the connector profile, after which the pairing is complete. 

While paired the remote periodically updates the controller every 1/100th of a second (10ms). If there is no update for 50ms the controller enters the disconnected state, in which case all motors are powered off and lights flash red. Upon regaining connection, the remote must acknowledge the disconnect before the motors can be powered on again. 

## Power


The controller brick operates from any voltage between 5 and 9 volts, inclusive. It uses a special connector that supports connecting 2 Li-Ion cells in series as well as other configurations:

    1N    2P  
    1N 2N 2P
    1P 1P 2N

Internally the `1P` and `2N` terminals are connected together to provide 7.4V between `1N` and `2P` which should be used for other sources. 

The controller consists of ATTiny, radio and the following:

- 2 motor drivers (5 or 9volts)
- 5 extra peripherals

- motor driver: 2x https://www.pololu.com/product/2960
- 9V stepup: 

## Connectors

The connectors are built from headers, in a way that supports all possible orientations by doubling the pins. 

### Motors

Motors use 3x4 12pin connector that allows the motor to select its own voltage as well as draw voltage for electronics. 

    5V   +   -   9V
    SEL GND GND SEL
    9V   -   +   5V

### I/O


This includes Speaker, RGB lights, buttons... Arranged in 2-1 in a row. 

    5V    X
    GND GND
    X    5V

### Expander

The expander connector is a 7pin 3x2x3 connector with the following layout:

    SDA     SCL
    5V  GND  5V 
    SCL     SDA

> TODO should there be I2C connector? 

## Peripherals

Internally there will be two INA219 for measuring the current on the attached motors. An accelerometer is also a possibility. 

> See if I have enough accels and other useful I2C parts. 

### LEGO 4.5V Motor

Uses the motor connector, 5V input. 

### LEGO Power Functions 9V Motor

Uses the motor connector, 9V input. 

### LEGO NXT Motor 

Uses the motor connector, 9V input. Additionally can use up to 2 peripheral connectors for the speed control. 

- 1 (white) Motor PWR 1
- 2 (black) Motor PWR 2
- 3 (red) GND
- 4 (green) ~4.5V
- 5 (yellow) Encoder 1
- 6 (blue) Encoder 2

### LEGO Compatible servo

Uses the peripheral connector in servo mode. 

### PWM Speaker

Uses a dedicated peripheral connector that is wired to PWM supporting pin. 

### RGB LEDs (chainable)

Uses a peripheral connector

### Button (chainable)

Uses a peripheral connector. Chaining works as or so that any chained button press will register as press. 

# Remote Protocol

# RCKid Remote Protocol

A simple and extensible protocol that allows the RCKid to use its onboard NRF radio to remotely control various devices. The RCKid acts as a _remote_ that can control an arbitrary number of _devices_. Each device has a number of _channels_ into which all the device's functionality has to be encoded. The remote protocol supports pairing, querying devices and their channel capabilities and so on. 

> Note that the protocol is very simple and its main purpose is *not* security. 

## Channels

A client device is described by a list of channels. Each channel has a specific type and consists of the following distinct parts:

- _control_, which is information the remote communicates regularly to the device that alters the state of the channel (such as motor speed)
- _feedback_, which is information the device communicates to the remote, such as current consumption
- _config_, which consists of channel configuration that is not expected to be changed once the remote and the device are paired, such as the motor's current limit

Each part of the channel can be empty, depending on the channel type. Channels are identified by numbers from `1` to `255` inclusive. The following channel types are supported:

### Motor channel

The motor channel provides control for single DC brushed motor attached to an H bridge. It consists



### CustomIO

A configurable channel that can fit multiple purposes. Corresponds to a single wire and various digital/analog functions it may have, namely:

Mode              | Control   | Feedback | Config
------------------|-----------|----------| -------------
Digital Out       | 0/1       |          | 00
Digital In        |           | 0/1      | 01
Analog Out (DAC)  | 0 - 255   |          | 02
Analog In (ADC)   |           | 0 - 255  | 03
PWM               | 0 - 255   |          | 04
Servo             | 0 - 65535 |          | 05 iiii xxxx
Tone              | 0 - 65535 |          | 06

### ToneEffect



### RGB Strip

The RGB strip channel is designed to configure and control a strip of RGB colors such as neopixels. The config part of the strip is the index of the first RGB channel specifying the color and the length of the strip. The length of the strip should not exceed the number of RGB colors available. The is no feedback and the channel control is the state of the strip, which can be one of:

ID   | Meaning
-----|-------------------
`00` | Manual control - the strip will display exactly the colors specified in the RGB channels
`..` | Effect - the number specifies which effect to use

### RGB Color

Single RGB color. Its control cosists of the three RGB bytes. No feedback or config is possible. The purpose of the RGB color channel is to simply hold memory for one neopixel. The device itself and the RGBStrip channel's config the determine which of these colors will be used. 

## Communication




## Commands

By default the remote sends the device channel control information and the device sends back the channel feedback info when there is change. Instead of sending directly the channel data, the remote may choose to send one of the predfined commands instead:

`00 00` - Heartbeat

`00 01 xx` - Read feedback of channel `xx`

`00 02 xx` - Read control of channel `xx`


    