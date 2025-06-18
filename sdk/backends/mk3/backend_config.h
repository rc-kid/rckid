#pragma once
/** Display width & height for the UI. 
 
    Although in config, DO NOT change those values unless you have a very good reason to do so. 
 */
#define RCKID_DISPLAY_WIDTH 320 
#define RCKID_DISPLAY_HEIGHT 240

#define RCKID_SPLASHSCREEN_OFF 0

/** Default display brightness. 
 */
#define RCKID_DISPLAY_BRIGHTNESS 128

/** Idle timer in seconds, after which the device powers off. 
 */
#define RCKID_IDLE_TIMETOUT 300

/** Idle timeout fallback that is not affected by the keepalive function call. 
 */
#define RCKID_IDLE_TIMETOUT_KEEPALIVE 1800

/** Frames per long press of the home button (at 60 fps). The home button long press is automatically detected by the AVR and will either turn the device on when powered off, or will force it to shutdown mode if powered on. 
 */
#define RCKID_HOME_BUTTON_LONG_PRESS_FPS 120

/** Default brightness of the RGB LEDs under buttons. Can be anything from 0 (off) to 255 (maximum brightess). High settings affect battery consumption quite a lot and are often not necessary.
 */
#define RCKID_RGB_LED_DEFAULT_BRIGHTNESS 32

/** Default values for the rumbler effects. 
 
    TODO fill in meaningful values and units.
 */
#define RCKID_RUMBLER_DEFAULT_STRENGTH 128
#define RCKID_RUMBLER_OK_STRENGTH 128
#define RCKID_RUMBLER_OK_TIME_ON 5
#define RCKID_RUMBLER_OK_TIME_OFF 1
#define RCKID_RUMBLER_OK_CYCLES 1
#define RCKID_RUMBLER_FAIL_STRENGTH 255
#define RCKID_RUMBLER_FAIL_TIME_ON 5
#define RCKID_RUMBLER_FAIL_TIME_OFF 5
#define RCKID_RUMBLER_FAIL_CYCLES 3
#define RCKID_RUMBLER_NUDGE_STRENGTH 128
#define RCKID_RUMBLER_NUDGE_TIME_ON 5
#define RCKID_RUMBLER_NUDGE_TIME_OFF 1
#define RCKID_RUMBLER_NUDGE_CYCLES 1

/** When enabled, all logging information will be pushed not only to the USB tty, but also to hardware serial port connected to the given TX and RX pins. By default they correspond to the first 2 cartridge pins. Only uart0 is available for this purpose, and will be locked for the SDK usage as only uart0 pins are exposed on the cartridge.
 */
#define RCKID_LOG_TO_SERIAL 1
#define RCKID_LOG_SERIAL_TX_PIN 12
#define RCKID_LOG_SERIAL_RX_PIN 13

#define RCKID_WAIT_FOR_SERIAL 0


/** When turned on, enables stack protection where magic values are stored to the end of stack and are periodically checked for corruption to give an early warning in case of stack underflow into the heap/arena area.
 */
#define RCKID_ENABLE_STACK_PROTECTION 1
/** Slightly lower than the 4096 bytes actually available per core stack to get the warning earlier-ish.
 */
#define RCKID_STACK_LIMIT_SIZE 4000
/** Actually the beginning of stack for core 0, but end of stack region for incremental memory mapping.
 */
#define RCKID_STACK_END 0x20082000