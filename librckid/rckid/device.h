#pragma once

namespace rckid {

    /** RCKid device class. 
     
        Encapsulates the state of the device and its basic peripherals. A static class is used to maintain encapsulation while providing inlinable, fast implementations of the friend functions from the RCKid's API. 
     */
    class Device {
    public:
    private:
        friend class BaseApp;
        friend class audio;

        /** Pointer to the end of allocated VRAM. 
         */
        static inline uint32_t * vramNext_ = 0;

        static inline uint32_t ticks_ = 0;

        static inline DeviceState state_;
        static inline State lastState_;

        static inline int16_t accelX_;
        static inline int16_t accelY_;
        static inline int16_t accelZ_;
        static inline int16_t gyroX_;
        static inline int16_t gyroY_;
        static inline int16_t gyroZ_;

        static inline uint16_t lightALS_ = 0;
        static inline uint16_t lightUV_ = 0;
       
        template<typename T>
        static void sendCommand(T const & cmd) {
            /// TODO: ensure T is a command
            i2c_write_blocking(i2c0, AVR_I2C_ADDRESS, (uint8_t const *) & cmd, sizeof(T), false);
        }    

        static void initialize();

        /** Initializes the RCKid. 
         
            Starts the chip and ist subsystems, I2C communication with the AVR and other peripherals and the display. Sets up any necessary structures and memory and then calls the rckid_main function which is never expected to return. 
        */
        friend void start();

        /** Updates the device by talking to all common peripherals, etc. 
         
            For each tick we need to get the following:

            - AVR status - 6 bytes for the buttons, info and config 
            - Sensors

            Each can be programmed by 
         
         */
        static void tick();

        static bool btnDown(Btn btn, State const & state) {
            switch (btn) {
                case Btn::Left:
                    return state.btnLeft();
                case Btn::Right:
                    return state.btnRight();
                case Btn::Up:
                    return state.btnUp();
                case Btn::Down:
                    return state.btnDown();
                case Btn::A:
                    return state.btnA();
                case Btn::B:
                    return state.btnB();
                case Btn::Select:
                    return state.btnSel();
                case Btn::Start:
                    return state.btnStart();
                case Btn::VolumeUp:
                    return state.btnVolUp();
                case Btn::VolumeDown:
                    return state.btnVolDown();
                case Btn::Home:
                    return state.btnHome();
                default:
                    // unreachable
                    return false;
            }
        }

        friend __force_inline void fatalError(int code, char const * file, int line) { 
            fatalErrorFile_ = file;
            fatalErrorLine_ = line;
            longjmp(fatalError_, code); 
        }

        // basic functions
        friend void yield();
        
        // power management
        friend void powerOff();
        friend bool charging() { return state_.state.charging(); }
        friend bool dcPower() { return state_.state.dcPower(); }
        friend unsigned vcc() { return state_.state.vcc(); }
        // brightness, notifications & LEDs
        friend void setBrightness(uint8_t brightness) { 
            Device::sendCommand(cmd::SetBrightness(brightness)); 
        }
        friend void disableLEDs() { 
            Device::sendCommand(cmd::RGBOff{}); 
        }
        friend void setButtonEffect(Btn btn, RGBEffect effect) {
            uint8_t index;
            switch (btn) {
                case Btn::B:
                    index = 0;
                    break;
                case Btn::A:
                    index = 1;
                    break;
                case Btn::Left:
                case Btn::Right:
                case Btn::Up:
                case Btn::Down:
                    index = 3;
                    break;
                case Btn::Select:
                    index = 4;
                    break;
                case Btn::Start:
                    index = 5;
                    break;
                default:
                    // TODO can't really do anything here
                    return;
            }
            Device::sendCommand(cmd::SetRGBEffect{index, effect});
        }

        friend void setButtonsEffects(RGBEffect a, RGBEffect b, RGBEffect dpad, RGBEffect sel, RGBEffect start) {
            Device::sendCommand(cmd::SetRGBEffects{a, b, dpad, sel, start});
        }



        // controls & sensors
        friend bool down(Btn b) { return btnDown(b, state_.state); }
        friend bool pressed(Btn b) { return btnDown(b, state_.state) && ! btnDown(b, lastState_); }
        friend bool released(Btn b) { return !btnDown(b, state_.state) && btnDown(b, lastState_); }
        friend int16_t accelX() { return accelX_; }
        friend int16_t accelY() { return accelY_; }
        friend int16_t accelZ() { return accelZ_; }
        friend int16_t gyroX() { return gyroX_; }
        friend int16_t gyroY() { return gyroY_; }
        friend int16_t gyroZ() { return gyroZ_; }
        friend uint16_t lightAmbient() { return lightALS_; }
        friend uint16_t lightUV() { return lightUV_; }
        friend unsigned tempAvr() { return state_.state.temp(); }

        // time utilities
        friend TinyDate time() { return state_.time; }

        // memory management
        friend size_t freeHeap();
#if (! defined LIBRCKID_MOCK)
        // on RP2040 we can use the linker script to deal with these function in the header file 
        friend size_t freeVRAM() { return &__vram_end__ - vramPtr_; }
        friend void resetVRAM() { vramPtr_ = &__vram_start__; }
        friend bool isVRAMPtr(void * ptr) { 
            return (ptr >= static_cast<void*>(& __vram_start__)) 
                   && (ptr < static_cast<void*>(& __vram_end__)); 
        }        
#else
        friend size_t freeVRAM();
        friend void resetVRAM();
        friend bool isVRAMPtr(void * ptr);
#endif
        friend void * allocateVRAM(size_t numBytes) {
            if (numBytes % 4 != 0)
                numBytes += 4 - (numBytes % 4);
            if (numBytes > freeVRAM())
                FATAL_ERROR(VRAM_OUT_OF_MEMORY);
            void * result = static_cast<void*>(Device::vramPtr_);
            // it's ok to disable the array bounds check here as we have checked the size already above
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
            Device::vramPtr_ += numBytes;
#pragma GCC diagnostic pop            
            return result;
        }
        static inline uint8_t * vramPtr_ = nullptr;

        static void BSOD(int code);

        static inline jmp_buf fatalError_;
        static inline int fatalErrorLine_ = 0;
        static inline char const * fatalErrorFile_ = nullptr;

    }; 


} // namespace rckid