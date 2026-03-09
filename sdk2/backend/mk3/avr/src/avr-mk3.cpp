/** AVR Firmware for RCKid
 
    This is the firmware for ATTiny3217 in RCKid mkIII, specifically SDK version 1.0 and up. The main difference from the previous mkIII versions is simpler execution model with fewer commands and firmware logic as per the SDK 1.0 refactoring.


 */

#include "avr-commands.h"

using namespace rckid;

class RCKid {
public:

    static void initialize() {

    }

    static void loop() {

    }

    /** \name I2C Communication
     
        - maybe have transferrable state that returns state, date time, all of user bytes
        - reset at the end of tx to state_ 
        - then have command sthat do things 
     */
    //@{

    static inline volatile bool irq_ = false;
    static inline volatile bool i2cCommandReady_ = false;
    static inline uint8_t * i2cRxAddr_ = nullptr;
    static inline uint8_t i2cBytes_ = 0;
    static inline uint8_t i2cBuffer_[16];

    static void initializeComms() {
        i2cRxAddr_ = reinterpret_cast<uint8_t *>(& ts_);
    }

    static void i2cSlaveIRQHandler() {
        #define I2C_DATA_MASK (TWI_DIF_bm | TWI_DIR_bm) 
        #define I2C_DATA_TX (TWI_DIF_bm | TWI_DIR_bm)
        #define I2C_DATA_RX (TWI_DIF_bm)
        #define I2C_START_MASK (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
        #define I2C_START_TX (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
        #define I2C_START_RX (TWI_APIF_bm | TWI_AP_bm)
        #define I2C_STOP_MASK (TWI_APIF_bm | TWI_DIR_bm)
        #define I2C_STOP_TX (TWI_APIF_bm | TWI_DIR_bm)
        #define I2C_STOP_RX (TWI_APIF_bm)
        uint8_t status = TWI0.SSTATUS;
        // sending data to accepting master simply starts sending the ts_.state buffer. 
        if ((status & I2C_DATA_MASK) == I2C_DATA_TX) {
            TWI0.SDATA = i2cRxAddr_[i2cBytes_++];
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            if (i2cRxAddr_ == reinterpret_cast<uint8_t *>(& ts_) && i2cBytes_ == 4) {
                irq_ = false;
            }
            /*
            TWI0.SDATA = ((uint8_t*) & state_)[i2cTxIdx_];
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            // clear IRQ once we read the state 
            if (++i2cTxIdx_ == 3) {
                irq_ = false;
#if !AVR_INT_IS_SERIAL_TX                
                // only change the pin state if we are not using it for serial TX
                gpio::outputFloat(AVR_PIN_AVR_INT);
#endif
                // clear all our interrupts - RP2350 is now in charge of keepin them
                state_.status.clearAllInterrupts();
            }
                */
            // TODO send nack when done sending all state
        // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
        } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
            i2cBuffer_[i2cBytes_++] = TWI0.SDATA;
            TWI0.SCTRLB = (i2cBytes_ == sizeof(i2cBuffer_)) ? TWI_SCMD_COMPTRANS_gc : TWI_SCMD_RESPONSE_gc;
        // master requests slave to write data, reset the sent bytes counter, initialize the actual read address from the read start and reset the IRQ
        } else if ((status & I2C_START_MASK) == I2C_START_TX) {
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc + TWI_SCMD_RESPONSE_gc;
            i2cBytes_ = 0;
        // master requests to write data itself. ACK if there is no pending I2C message, NACK otherwise. The buffer is reset to 
        } else if ((status & I2C_START_MASK) == I2C_START_RX) {
            TWI0.SCTRLB = (! i2cCommandReady_) ? TWI_SCMD_RESPONSE_gc : TWI_ACKACT_NACK_gc;
        // sending finished, reset the tx address and when in recording mode determine if more data is available
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            i2cRxAddr_ = reinterpret_cast<uint8_t *>(& ts_);
        // receiving finished, inform main loop we have message waiting if we have received at laast one byte (0 bytes received is just I2C ping)
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            if (i2cBytes_ > 0)
                i2cCommandReady_ = true;
        } else {
            // error - a state we do not know how to handle
        }
    }

    static void processI2CCommand() {

    }
    //@}

    static inline TransferrableState ts_;

}; // RCKid

/** I2C slave action.
 */
ISR(TWI0_TWIS_vect) {
    RCKid::i2cSlaveIRQHandler();    
}


int main() {
    RCKid::initialize();
    while (true)
        RCKid::loop();
}
