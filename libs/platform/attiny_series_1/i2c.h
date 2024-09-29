namespace i2c {

    inline void disableSlave() {
        TWI0.SCTRLA = 0;
    }

    inline void disableMaster() {
        TWI0.MCTRLA = 0;
    }

    inline void disable() {
        TWI0.SCTRLA = 0;
        TWI0.MCTRLA = 0;
    }

    inline void initializeSlave(uint8_t address) {
        // turn I2C off in case it was running before
        disableSlave();
        //TWI0.MCTRLA = 0;
        // make sure that the pins are not out - HW issue with the chip, will fail otherwise
        if (PORTMUX.CTRLB | PORTMUX_TWI0_bm)
            PORTA.OUTCLR = 0x06; // PA1, PA2
        else
            PORTB.OUTCLR = 0x03; // PB0, PB1
        // set the address and disable general call, disable second address and set no address mask (i.e. only the actual address will be responded to)
        TWI0.SADDR = address << 1;
        TWI0.SADDRMASK = 0;
        // enable the TWI in slave mode, don't enable interrupts as the default implementation operates in polling mode
        TWI0.SCTRLA |= TWI_ENABLE_bm;
        // TWI0.SCTRLA = TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm  | TWI_ENABLE_bm;

        // bus Error Detection circuitry needs Master enabled to work 
        // not sure why we need it
        TWI0.MCTRLA = TWI_ENABLE_bm;   
    }

    inline void initializeMaster() {
        cli();
        // turn I2C off in case it was running before
        disableMaster();
        // TODO disable slave as well? 
        // make sure that the pins are nout out - HW issue with the chip, will fail otherwise
        if (PORTMUX.CTRLB | PORTMUX_TWI0_bm)
            PORTA.OUTCLR = 0x06; // PA1, PA2
        else
            PORTB.OUTCLR = 0x03; // PB0, PB1
        uint32_t baud = ((F_CPU/100000) - (((F_CPU* /* rise time */300)/1000)/1000)/1000 - 10)/2;
        TWI0.MBAUD = (uint8_t)baud;
        TWI0.MCTRLA = TWI_ENABLE_bm;         
        TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
        sei();
    }

    inline bool masterTransmit(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize) {
        auto wait = [](){
            while (!(TWI0.MSTATUS & (TWI_WIF_bm | TWI_RIF_bm))) {}; 
        };
        auto waitIdle = [](){
            while (!(TWI0.MSTATUS & TWI_BUSSTATE_IDLE_gc)) {}; 
        };

        auto busLostOrError = [](){
            return TWI0.MSTATUS & (TWI_BUSERR_bm | TWI_ARBLOST_bm);
        };
        auto stop = [&](){
            TWI0.MCTRLB = TWI_MCMD_STOP_gc;
            waitIdle();
        }; 
        auto start = [&](uint8_t address, bool read) {
            TWI0.MADDR = (address << 1) | read;
            wait();
            if (busLostOrError()) {
                waitIdle();
                return false;
            }
            if (TWI0.MSTATUS & TWI_RXACK_bm) {
                stop();
                return false;
            }
            return true;
        };
        if (wsize > 0) {
            if (! start(address, false)) 
                goto i2c_master_error;
            // we send all we have no matter what 
            for (uint8_t i = 0; i < wsize; ++i) {
                TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;                    
                TWI0.MDATA = *(wb++); 
                wait();
                if (busLostOrError()) {
                    stop();
                    goto i2c_master_error;
                }
            }
            if (rsize == 0)
                stop();
        }
        if (rsize > 0) {
            if (! start(address, true))
                goto i2c_master_error;
            while (rsize-- > 0) {
                wait();
                *(rb++) = TWI0.MDATA;
                TWI0.MCTRLB = (rsize > 0) ? TWI_MCMD_RECVTRANS_gc : TWI_ACKACT_NACK_gc;
            }
            stop();
        }
        return true;
    i2c_master_error:
        TWI0.MCTRLA = TWI_FLUSH_bm;
        return false;
    }

#ifdef FOO
private:

    
    static void stop() {
    }

    static void wait() {
    }

    static void waitIdle() {
    }

    static bool busLostOrError() {
    }

#endif

}; // i2c