    #ifndef COMMAND
    #error "COMMAND macro must be defined before inserting"
    // just for pretty coloring below
    #define COMMAND(...)
    #endif

    /** No operation. 
     
        Also resets the read address to the beginning of the device status (default).
     */
    COMMAND(0, Nop)

    /** Returns the version of the AVR firmware.
     */
    COMMAND(1, GetVersion)
    
    /** Sets the read address for the next I2C read to the beginning of the time information.
     */
    COMMAND(2, GetTime)
    
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
        WriteStorage(uint16_t offset, uint8_t const * data, uint32_t numBytes): offset{offset}, numBytes{numBytes} {
            ASSERT(len <= sizeof(this->data));
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

    COMMAND(18, SetBrightness,
        uint8_t value;
        SetBrightness(uint8_t value): value{value} {}
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

    #undef COMMAND
