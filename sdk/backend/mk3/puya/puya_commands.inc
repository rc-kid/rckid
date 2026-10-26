    #ifndef COMMAND
    #error "COMMAND macro must be defined before inserting"
    // just for pretty coloring below
    #define COMMAND(...)
    #endif

    /** No operation. 
     */
    COMMAND(0, Nop)

    /** Enters the power down state if powered on. When sent second time (i.e. while in the PowerDown mode) will immediately turn the device off. When used in the Alarm state, powers the device off immediately (since in the alarm state the user input reading main loop at RP2350 is not supposed to run)
     */
    COMMAND(1, PowerOff)

    /** Acknowledges the PowerDown state transition, which extends the power down state timeout to the extended value. Can only be used once. Further acknowledges will not have any effect.
     */
    COMMAND(2, PowerDownAck)

    /** Useful only in the Alarm state, makes the device to enter normal power on state. No effect in any other state.
     */
    COMMAND(3, PowerOn)

    /** Forces hardware reboot of the RP2350 and cartridge by cycling the power. 
     */
    COMMAND(4, RebootRP);

    /** Reboots the RP2350 in bootloader mode so that when connected to a PC it will present the cartridge's flash as UF2 device.
     */
    COMMAND(5, BootloaderRP);

    COMMAND(6, SetAlarm,
        uint32_t secondsFromNow;
        uint32_t reason;
        explicit SetAlarm(uint32_t value, uint32_t reason = 0): secondsFromNow{value}, reason{reason} {}
    )

    /** Sets the date & time kept by the RTC.
     */
    COMMAND(7, SetTime,
        TinyDateTime value;
        explicit SetTime(TinyDateTime value): value{value} {}
    )


    #ifdef FOOBAR
    
    /** Sets the date & time kept by the AVR.
     */
    COMMAND(3, SetTime,
        TinyDateTime value;
        explicit SetTime(TinyDateTime value): value{value} {}
    )

    /** Sets the read address for the next I2C read to the given offset in the 1024 bytes of storage data. 
     */
    COMMAND(4, ReadStorage,
        uint16_t offset;
        explicit ReadStorage(uint16_t offset = 0): offset{offset} {}
    )

    /** Writes bytes to the storage buffer at given offset.
     
     */
    COMMAND(5, WriteStorage,
        uint16_t offset;
        uint8_t numBytes;
        uint8_t data[RCKID_I2C_MAX_ASYNC_MSG_SIZE - sizeof(offset) - sizeof(numBytes) - 1];
        WriteStorage(uint16_t offset, uint8_t const * data, uint32_t numBytes): offset{offset}, numBytes{static_cast<uint8_t>(numBytes)} {
            ASSERT(numBytes <= sizeof(this->data));
            memcpy(this->data, data, numBytes);
        }
    )

    COMMAND(6, ReadEEPROM) // TODO
    COMMAND(7, WriteEEPROM) // TODO

    // power management commands

    /** Immediately powers off the device by cutting the 3V3 power supply for the cartridge and RP2350.
     */
    COMMAND(8, PowerOff)

    COMMAND(9, PowerOffAck)

    COMMAND(10, Sleep) // TODO

    COMMAND(11, WakeUp, 
        uint32_t countdownSeconds;
        uint32_t reason;
        explicit WakeUp(uint32_t countdownSeconds, uint32_t reason): countdownSeconds{countdownSeconds}, reason{reason} {}
    )

    COMMAND(12, RebootRP);

    COMMAND(13, BootloaderRP);

    COMMAND(14, RebootAVR); // TODO
    COMMAND(15, BootloaderAVR); // TODO

    COMMAND(16, SetDebugMode, 
        bool value;
        explicit SetDebugMode(bool value): value{value} {}
    )

    COMMAND(17, SetUartDebug,
        bool value;
        explicit SetUartDebug(bool value): value{value} {}
    )

    COMMAND(19, SetRGBEffectAll,
        RGBEffect effect;
        explicit SetRGBEffectAll(RGBEffect effect): effect{effect} {}
    )

    COMMAND(20, SetRGBEffect,
        uint8_t ledIndex;
        RGBEffect effect;
        SetRGBEffect(uint8_t ledIndex, RGBEffect effect): ledIndex{ledIndex}, effect{effect} {}
    )

    COMMAND(21, SetRumblerEffect,
        RumblerEffect effect;
        explicit SetRumblerEffect(RumblerEffect effect): effect{effect} {}
    )

    #endif

    #undef COMMAND
