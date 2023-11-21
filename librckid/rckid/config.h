




/** I2C address of the AVR chip (always slave)
 */
#define AVR_I2C_ADDRESS 0x43

/** Duration of the home button press necessasy to power the device on. Measured in ADC cycles for the initial voltage detection, i.e. 1302 cycles per second. 
 */
#define BTN_HOME_POWER_ON_DURATION 1302

#define VCC_DC_POWER_THRESHOLD 430
#define CHARGING_DETECTION_THRESHOLD 255
/** Headphones detection threshold (when no headphones are inserted, the line is pulled up to 3V, i.e. some 930. 
 */
#define HEADPHONES_DETECTION_THRESHOLD 512

/** Determines the cartridge detection button is wired and working. If true, the device will not turn on if a cartridge is not present.  
*/
#define CARTRIDGE_DETECTION_ENABLED true