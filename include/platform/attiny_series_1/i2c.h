namespace i2c {

    static void disableSlave() {
        TWI0.SCTRLA = 0;
    }

    static void initializeSlave(uint8_t address) {
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

}