# ESP8266 Things

The ESP8266 is used in either a cartridge, in which case it communicates with the RCKid directly over an SPI bus, or as a base station, where the WiFi connection provided by the cartridge module is wrapped in a radio layer that allows NRF or LoRa communication to be used to access the internet as well. 

The functionality comprises of the following basic modules that are descibed in greater detail below:

- configuration for detecting available networks, connecting and disconnecting, etc. 
- http for basic http get/post functionality
- telegram for messaging

This is a bit more complicated with the basestation, whereas the single wifi is shared between multiple devices. 

## Configuration

## HTTP Connections

## Telegram Support

Each RCKid device can act as Telegram bot and be part of chats. 
