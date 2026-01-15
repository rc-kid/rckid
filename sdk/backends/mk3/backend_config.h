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

/** Default brightness of the RGB LEDs under buttons. Can be anything from 0 (off) to 255 (maximum brightess). High settings affect battery consumption quite a lot and are often not necessary.
 */
#define RCKID_RGB_BRIGHTNESS 32

/** Idle timer in seconds, after which the device powers off. 
 */
#define RCKID_IDLE_TIMEOUT 300

/** Heartbeat mode is periodic wakeup when the device is not used during which the device can process periodic tasks such as checking for new messages, synchronizing time, etc. To disable heartbeats altogether, set this value to 0.
 */
#define RCKID_HEARTBEAT_PERIOD 180

/** Heartbeat service timeout. If the heartbeat task takes longer than this number of ticks (60fps) and the device is *not* turned on by the user, the device turns itself off forcefully.
 */
#define RCKID_HEARTBEAT_TIMEOUT_FPS (60 * 60)

/** Idle timeout fallback that is not affected by the keepalive function call. I.e. no matter what the application does, after this time, the device will power itself off. Note that when in debug mode, the device is automatically in keepalive mode for all applications.
 */
#define RCKID_IDLE_TIMEOUT_KEEPALIVE 3600

/** Frames per long press of the home button (at 60 fps). The home button long press is automatically detected by the AVR and will either turn the device on when powered off, or will force it to shutdown mode if powered on. 
 */
#define RCKID_HOME_BUTTON_LONG_PRESS_FPS 120

/** Timeout for the RP2350 to issue power off command *after* the power off interrupt has been issued. If power off command is not received in this time, the AVR will forcibly power off the device anyways.
 */
#define RCKID_POWEROFF_TIMEOUT_FPS (10 * 60)

/** Timeout for the RP2350 to acknowledge the power interrupt request.  
 */
#define RCKID_POWEROFF_ACK_TIMEOUT_FPS 10

/** Default speed of the breathing effect for system notifications (power, debug mode, purple, etc.)
 */
#define RCKID_RGB_NOTIFICATION_SPEED 6

/** Have the rapid fire trigger roughly every 250ms. 
 */
#define RCKID_DEFAULT_RAPIDFIRE_TICKS 6

#define RCKID_BUTTON_DEBOUNCE_TICKS 4

/** Default values for the rumbler effects. 
 
    TODO fill in meaningful values and units.
 */
#define RCKID_RUMBLER_DEFAULT_STRENGTH 128
#define RCKID_RUMBLER_OK_STRENGTH 192
#define RCKID_RUMBLER_OK_TIME_ON 5
#define RCKID_RUMBLER_OK_TIME_OFF 1
#define RCKID_RUMBLER_OK_CYCLES 1
#define RCKID_RUMBLER_FAIL_STRENGTH 255
#define RCKID_RUMBLER_FAIL_TIME_ON 5
#define RCKID_RUMBLER_FAIL_TIME_OFF 5
#define RCKID_RUMBLER_FAIL_CYCLES 3
#define RCKID_RUMBLER_NUDGE_STRENGTH 128
#define RCKID_RUMBLER_NUDGE_TIME_ON 5
#define RCKID_RUMBLER_NUDGE_TIME_OFF 0
#define RCKID_RUMBLER_NUDGE_CYCLES 1


/** When enabled, all logging information will be pushed not only to the USB tty, but also to hardware serial port connected to the given TX and RX pins. By default they correspond to the first 2 cartridge pins. Only uart0 is available for this purpose, and will be locked for the SDK usage as only uart0 pins are exposed on the cartridge.
 */
#define RCKID_LOG_TO_SERIAL 0
#define RCKID_LOG_SERIAL_TX_PIN 12
#define RCKID_LOG_SERIAL_RX_PIN 13

#define RCKID_WAIT_FOR_SERIAL 0


/** When turned on, enables stack protection where magic values are stored to the end of stack and are periodically checked for corruption to give an early warning in case of stack underflow into the heap/arena area.
 */
#define RCKID_ENABLE_STACK_PROTECTION 1

#define RGB_LED_DPAD_TOP_LEFT 1
#define RGB_LED_DPAD_TOP_RIGHT 0
#define RGB_LED_DPAD_BOTTOM_LEFT 2
#define RGB_LED_DPAD_BOTTOM_RIGHT 3
#define RGB_LED_BTN_A 7
#define RGB_LED_BTN_B 6
#define RGB_LED_BTN_SELECT 4
#define RGB_LED_BTN_START 5