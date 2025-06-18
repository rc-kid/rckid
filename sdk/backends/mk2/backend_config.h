#pragma once
/** \page backend_mk2 RCKid mkII Backend
 
    The first RCKid device with RP2040 chip (called mkII because of the mkI prototype based on RPi zero). The latest review of the mkII device is 2.2, which has a few hardware issues (below). The hardware supports PWM sound output at 12bits and 44.1kHz, PDM microphone input and 2.8" 320x240 display in 8bit MCU interface. The device has a LiPo charger and battery monitoring, as well as a few other features.

    Note that this backend is now obsolete and willbe removed once the mkIII hardware is ready.

    \section Configuration

    The following macros can be used to influence the backend behavior:

    - RCKID_ENABLE_STACK_PROTECTION checks at random intervals that the bottom of the stack just above the heap has not been touched. This is especially important on the device where stack size is rather small. 
    - RCKID_LOG_TO_SERIAL enables logging to the hardware serial port at pins 16 & 17 (uart0) at 74880 baudrate, which can be read through the cartridge port. When not used, logs are sent to the USB serial port.
    - RCKID_WAIT_FOR_SERIAL blocks the device until the USB serial port is connected. This is useful for debugging, as it ensures that the device (after initialize() function) will not be started until after the USB serial is connected and a character is received, giving the connected PC time to start reading ensuring that even early messages will be displayed

    \section HardwareIssues Hardware Issues

    - USB voltage leaks through the USB protection chip. Software fix is manual enabling and disabling of the USB MSC feature via the DataSync app
    - When charging, AVR runs at 5V and the I2C high is 3.3V which is just at the edge of high detection, leading to unreliable I2C connection. As long as not used while charging, this is not a problem. 
 */


#define RCKID_LOG_TO_SERIAL 0
#define RCKID_WAIT_FOR_SERIAL 0

/** Idle timer in seconds, after which the device powers off. 
 */
#define RCKID_IDLE_TIMETOUT 300

/** Idle timeout fallback that is not affected by the keepalive function call. 
 */
#define RCKID_IDLE_TIMETOUT_KEEPALIVE 1800

/** Display width & height for the UI. 
 */
#define RCKID_DISPLAY_WIDTH 320 
#define RCKID_DISPLAY_HEIGHT 240


// copied from mk3 so that we can build
#define RCKID_RGB_LED_DEFAULT_BRIGHTNESS 32

#define RCKID_RUMBLER_DEFAULT_STRENGTH 1
#define RCKID_RUMBLER_OK_STRENGTH 1
#define RCKID_RUMBLER_OK_TIME_ON 1
#define RCKID_RUMBLER_OK_TIME_OFF 1
#define RCKID_RUMBLER_OK_CYCLES 1
#define RCKID_RUMBLER_FAIL_STRENGTH 1
#define RCKID_RUMBLER_FAIL_TIME_ON 1
#define RCKID_RUMBLER_FAIL_TIME_OFF 1
#define RCKID_RUMBLER_FAIL_CYCLES 1
#define RCKID_RUMBLER_NUDGE_STRENGTH 1
#define RCKID_RUMBLER_NUDGE_TIME_ON 1
#define RCKID_RUMBLER_NUDGE_TIME_OFF 1
#define RCKID_RUMBLER_NUDGE_CYCLES 1

#define RCKID_ENABLE_STACK_PROTECTION 1
#define RCKID_STACK_LIMIT_SIZE 4000
#define RCKID_STACK_END 0x20042000