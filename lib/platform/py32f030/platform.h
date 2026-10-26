#pragma once

#include <py32f0xx_ll_bus.h>
#include <py32f0xx_ll_gpio.h>
#include <py32f0xx_ll_rcc.h>
#include <py32f0xx_ll_usart.h>
#include <py32f0xx_ll_utils.h>
#include <py32f0xx_ll_tim.h>
#include <py32f0xx_ll_rtc.h>
#include <py32f0xx_ll_pwr.h>
#include <py32f0xx_ll_exti.h>

/** PY32F030 Platform Implementation
 */


class cpu {
public:

    enum class ClockSpeed : uint32_t {
        MHz4 = 4000000,
        MHz8 = 8000000,
        MHz16 = 16000000,
        MHz24 = 24000000,
    };

    /** Enables the */
    static void initialize(ClockSpeed speed = ClockSpeed::MHz8) {

        // Enable HSI (cannot fail)
        LL_RCC_HSI_Enable();
        switch (speed) {
            case ClockSpeed::MHz4:
                LL_RCC_HSI_SetCalibFreq(LL_RCC_HSICALIBRATION_4MHz);
                break;
            case ClockSpeed::MHz8:
                LL_RCC_HSI_SetCalibFreq(LL_RCC_HSICALIBRATION_8MHz);
                break;
            case ClockSpeed::MHz16:
                LL_RCC_HSI_SetCalibFreq(LL_RCC_HSICALIBRATION_16MHz);
                break;
            case ClockSpeed::MHz24:
                LL_RCC_HSI_SetCalibFreq(LL_RCC_HSICALIBRATION_24MHz);
                break;
        }
        while(LL_RCC_HSI_IsReady() != 1);

        // Set AHB prescaler
        LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

        // Configure HSISYS as system clock source (cannot fail)
        LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSISYS);
        while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSISYS);

        // Set APB1 prescaler
        LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
        LL_Init1msTick(static_cast<uint32_t>(speed));

        // Update system clock global variable SystemCoreClock (can also be updated by calling SystemCoreClockUpdate function)
        LL_SetSystemCoreClock(static_cast<uint32_t>(speed));

        // enable GPIO clocks for all ports
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOF);

    }

    static void delayUs(uint16_t value) {
//        _delay_us(value);
    }

    static void delayMs(uint16_t value) {
        LL_mDelay(value);
    }

    static void sleep() {
/*
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
        sleep_cpu();
*/
    }

    static void reset() {
        // reset the device by writing to the software reset register
    }

    static void wdtReset() __attribute__((always_inline)) {
        //__asm__ __volatile__ ("wdr"::);
    }

    static void nop()  __attribute__((always_inline)) { __NOP(); }

//    #include "../common/cpu_common.h"

    /** Disables interrupts for the lifetime of the guard. The previous PRIMASK value is saved and restored, so that the guards can be nested (or used where interrupts are already disabled) without re-enabling interrupts prematurely.
     */
    class DisableInterruptsGuard {
    public:
        DisableInterruptsGuard():
            primask_{__get_PRIMASK()} {
            __disable_irq();
        }

        ~DisableInterruptsGuard() {
            release();
        }

        /** Restores the interrupt state from before the guard was created, ahead of the guard's destruction. Subsequent calls (and the destructor) have no effect.
         */
        void release() {
            if (active_) {
                __set_PRIMASK(primask_);
                active_ = false;
            }
        }

        DisableInterruptsGuard(DisableInterruptsGuard const &) = delete;
        DisableInterruptsGuard & operator=(DisableInterruptsGuard const &) = delete;
    private:
        uint32_t primask_;
        bool active_ = true;

    }; // cpu::DisableInterruptsGuard


};


class gpio {
public:

    enum class AlternateFunction : uint32_t {
        USART1_TX,
        TIM1_CH1,
        TIM1_CH2,
        TIM1_CH3,
        TIM1_CH4,
        TIM3_CH1,
        TIM3_CH2,
        TIM3_CH3,
        TIM3_CH4,
    }; // gpio::AlternateFunction

    enum class Pin {
        PF0 = 0x00, 
        PF1,
        PF2,
        PF3,
        PF4,
        PA0 = 0x10,
        PA1, 
        PA2, 
        PA3,
        PA4,
        PA5,
        PA6,
        PA7,
        PA8,
        PA9,
        PA10,
        PA11,
        PA12,
        PA13,
        PA14,
        PA15,
        PB0 = 0x20,
        PB1,
        PB2,
        PB3,
        PB4,
        PB5,
        PB6,
        PB7,
        PB8,
        Unused = 0xff
    };

    static constexpr Pin PF0 = Pin::PF0;
    static constexpr Pin PF1 = Pin::PF1;
    static constexpr Pin PF2 = Pin::PF2;
    static constexpr Pin PF3 = Pin::PF3;
    static constexpr Pin PF4 = Pin::PF4;
    static constexpr Pin PA0 = Pin::PA0;
    static constexpr Pin PA1 = Pin::PA1;
    static constexpr Pin PA2 = Pin::PA2;
    static constexpr Pin PA3 = Pin::PA3;
    static constexpr Pin PA4 = Pin::PA4;
    static constexpr Pin PA5 = Pin::PA5;
    static constexpr Pin PA6 = Pin::PA6;
    static constexpr Pin PA7 = Pin::PA7;
    static constexpr Pin PA8 = Pin::PA8;
    static constexpr Pin PA9 = Pin::PA9;
    static constexpr Pin PA10 = Pin::PA10;
    static constexpr Pin PA11 = Pin::PA11;
    static constexpr Pin PA12 = Pin::PA12;
    static constexpr Pin PA13 = Pin::PA13;
    static constexpr Pin PA14 = Pin::PA14;
    static constexpr Pin PA15 = Pin::PA15;
    static constexpr Pin PB0 = Pin::PB0;
    static constexpr Pin PB1 = Pin::PB1;
    static constexpr Pin PB2 = Pin::PB2;
    static constexpr Pin PB3 = Pin::PB3;
    static constexpr Pin PB4 = Pin::PB4;
    static constexpr Pin PB5 = Pin::PB5;
    static constexpr Pin PB6 = Pin::PB6;
    static constexpr Pin PB7 = Pin::PB7;
    static constexpr Pin PB8 = Pin::PB8;
    static constexpr Pin UNUSED = Pin::Unused;

    static constexpr GPIO_TypeDef * pinBank(Pin pin) {
        switch (static_cast<unsigned>(pin) >> 4) {
            case 0: return GPIOF;
            case 1: return GPIOA;
            case 2: return GPIOB;
            default: return nullptr;
        }
    }

    static constexpr uint32_t pinMask(Pin pin) {
        return 1U << (static_cast<unsigned>(pin) & 0xf);
    }

    static void setAsOutput(Pin pin) {
        LL_GPIO_SetPinMode(pinBank(pin), pinMask(pin), LL_GPIO_MODE_OUTPUT);
        LL_GPIO_SetPinOutputType(pinBank(pin), pinMask(pin), LL_GPIO_OUTPUT_PUSHPULL);
        LL_GPIO_SetPinSpeed(pinBank(pin), pinMask(pin), LL_GPIO_SPEED_FREQ_LOW);
    }

    static void setAsInput(Pin pin) {
        LL_GPIO_SetPinMode(pinBank(pin), pinMask(pin), LL_GPIO_MODE_INPUT);
        LL_GPIO_SetPinPull(pinBank(pin), pinMask(pin), LL_GPIO_PULL_NO);
    }

    static void setAsInputPullup(Pin pin) {
        LL_GPIO_SetPinMode(pinBank(pin), pinMask(pin), LL_GPIO_MODE_INPUT);
        LL_GPIO_SetPinPull(pinBank(pin), pinMask(pin), LL_GPIO_PULL_UP);
    }

    static void setAsInputPulldown(Pin pin) {
        LL_GPIO_SetPinMode(pinBank(pin), pinMask(pin), LL_GPIO_MODE_INPUT);
        LL_GPIO_SetPinPull(pinBank(pin), pinMask(pin), LL_GPIO_PULL_DOWN);
    }

    static void write(Pin pin, bool value) {
        if (value)
            LL_GPIO_SetOutputPin(pinBank(pin), pinMask(pin));
        else
            LL_GPIO_ResetOutputPin(pinBank(pin), pinMask(pin));
    }

    static bool read(Pin pin) {
        return LL_GPIO_IsInputPinSet(pinBank(pin), pinMask(pin));
    }

    /** Returns the alternate function id for the given pin and alternate function.
     
        Puya uses weird alternate function numbers where different alternate functions have different numbers for different pins. Those are from the Puya datasheet, sections 3.1, 3.2 and 3.3. 

        If the pin does not support the given alternate function, 0xff is returned.
     */
    static constexpr uint32_t getAlternateFunctionID(Pin pin, AlternateFunction af) {
        switch (af) {
            case AlternateFunction::USART1_TX:
                switch (pin) {
                    case gpio::PF1:
                        return LL_GPIO_AF8_USART1;
                    case gpio::PF3:
                        return LL_GPIO_AF0_USART1;
                    case gpio::PA2:
                        return LL_GPIO_AF1_USART1;
                    case gpio::PA7:
                        return LL_GPIO_AF8_USART1;
                    case gpio::PA9:
                        return LL_GPIO_AF1_USART1;
                    case gpio::PA10:
                        return LL_GPIO_AF8_USART1;
                    case gpio::PA14: // careful this is SWDCLK
                        return LL_GPIO_AF1_USART1;
                    case gpio::PB6:
                        return LL_GPIO_AF0_USART1;
                    case gpio::PB8:
                        return LL_GPIO_AF8_USART1;
                    default:
                        return 0xff;
                }
            case AlternateFunction::TIM1_CH1:
                switch (pin) {
                    case gpio::PA3:
                        return LL_GPIO_AF13_TIM1;
                    case gpio::PA8:
                        return LL_GPIO_AF2_TIM1;
                    default:
                        return 0xff;
                }
            case AlternateFunction::TIM1_CH2:
                switch (pin) {
                    case gpio::PA9:
                        return LL_GPIO_AF2_TIM1;
                    case gpio::PA13: // careful this is SWDIO
                        return LL_GPIO_AF13_TIM1;
                    case gpio::PB3:
                        return LL_GPIO_AF1_TIM1;
                    default:
                        return 0xff;
                }            
            case AlternateFunction::TIM1_CH3:
                switch (pin) {
                    case gpio::PA0:
                        return LL_GPIO_AF13_TIM1;
                    case gpio::PA10:
                        return LL_GPIO_AF2_TIM1;
                    case gpio::PB6:
                        return LL_GPIO_AF1_TIM1;
                    default:
                        return 0xff;
                }
            case AlternateFunction::TIM1_CH4:
                switch (pin) {
                    case gpio::PA1:
                        return LL_GPIO_AF13_TIM1;
                    case gpio::PA11:
                        return LL_GPIO_AF2_TIM1;
                    default:
                        return 0xff;
                }
            case AlternateFunction::TIM3_CH1:
                switch (pin) {
                    case gpio::PA2:
                        return LL_GPIO_AF13_TIM3;
                    case gpio::PA6:
                    case gpio::PB4:
                        return LL_GPIO_AF1_TIM3;
                    default:
                        return 0xff;
                }
            case AlternateFunction::TIM3_CH2:
                switch (pin) {
                    case gpio::PA5:
                        return LL_GPIO_AF13_TIM3;
                    case gpio::PA7:
                    case gpio::PB5:
                        return LL_GPIO_AF1_TIM3;
                    default:
                        return 0xff;
                }
            case AlternateFunction::TIM3_CH3:
                switch (pin) {
                    case gpio::PA4:
                    case gpio::PF3:
                        return LL_GPIO_AF13_TIM3;
                    case gpio::PB0:
                        return LL_GPIO_AF1_TIM3;
                    default:
                        return 0xff;
                }
            case AlternateFunction::TIM3_CH4:
                switch (pin) {
                    case gpio::PB1:
                        return LL_GPIO_AF1_TIM3;
                    default:
                        return 0xff;
                }
        }
        return 0xff;
    }

    static constexpr void configureAsGPIO(Pin pin) {
        LL_GPIO_InitTypeDef cfg = {0};
        cfg.Pin = gpio::pinMask(pin);
        // TODO verify those are defaults
        cfg.Mode = LL_GPIO_MODE_INPUT;
        cfg.Speed = LL_GPIO_SPEED_FREQ_LOW;
        cfg.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        LL_GPIO_Init(gpio::pinBank(pin), & cfg);
    }

    static void configureAsAlternate(Pin pin, AlternateFunction af) {
        LL_GPIO_InitTypeDef cfg = {0};
        cfg.Pin = gpio::pinMask(pin);
        cfg.Mode = LL_GPIO_MODE_ALTERNATE;
        switch (af) {
            case AlternateFunction::USART1_TX:
                cfg.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
                cfg.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
                cfg.Pull = LL_GPIO_PULL_UP;
                break;
            case AlternateFunction::TIM1_CH1:
            case AlternateFunction::TIM1_CH2:
            case AlternateFunction::TIM1_CH3:
            case AlternateFunction::TIM1_CH4:
                cfg.Speed      = LL_GPIO_SPEED_FREQ_HIGH;
                cfg.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
                cfg.Pull       = LL_GPIO_PULL_NO;
                break;
            default:
                cfg.Speed      = LL_GPIO_SPEED_FREQ_LOW;
                cfg.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
                cfg.Pull       = LL_GPIO_PULL_NO;
                break;
        }
        cfg.Alternate = gpio::getAlternateFunctionID(pin, af);
        //ASSERT(cfg.Alternate != 0xff);
        LL_GPIO_Init(gpio::pinBank(pin), & cfg);
    }

    #include "../common/gpio_common.h"

}; 


/** Serial (USART)
 
    A very simple serial TX driver mostly for debugging purposes. Although the chip has two USARTs, only the USART1 is supported by the driver so far.
 */

class serial {
public:

    static void initializeTx(uint32_t speed, gpio::Pin txPin) {
        // ASSERT(gpio::getAlternateFunctionID(txPin, gpio::AlternateFunction::USART1_TX) != 0xff);
        // enable USART1 clock
        LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_USART1);

        // GPIO configuration 
        gpio::configureAsAlternate(txPin, gpio::AlternateFunction::USART1_TX);

        // USART configuration
        LL_USART_InitTypeDef USART_InitStruct = {0};
        USART_InitStruct.BaudRate = speed;
        USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
        USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
        USART_InitStruct.Parity = LL_USART_PARITY_NONE;
        USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX;
        USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
        USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
        LL_USART_Init(USART1, & USART_InitStruct);
        // Configure as full duplex asynchronous mode
        LL_USART_ConfigAsyncMode(USART1);
        // Enable UART module
        LL_USART_Enable(USART1);
    }

    static void waitForTx() {
        while(LL_USART_IsActiveFlag_TXE(USART1) != 1);
    }

    static void write(char c) {
        waitForTx();
        LL_USART_TransmitData8(USART1, c);
    }

}; // serial

/** Advanced Timer 1
 
    Simple interface to the timer. The timer can drive 4 PWM channels from single clock, with different duty cycles. At the moment the platform provides independent control of the channels for the PWM output only, but the timer can do a lot more in the future.
 */
class timer1 {
public:

    static void enable(uint32_t resolution = 256, uint32_t prescaler = 1) {
        LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_TIM1);

        LL_TIM_SetClockDivision(TIM1, LL_TIM_CLOCKDIVISION_DIV1);
        LL_TIM_SetCounterMode(TIM1, LL_TIM_COUNTERMODE_UP);
        LL_TIM_SetPrescaler(TIM1, prescaler - 1);
        LL_TIM_SetAutoReload(TIM1, resolution - 1);
        LL_TIM_SetRepetitionCounter(TIM1, 0);
        // prescaler & repetition counter are only loaded on update event, force one so that the first period is correct too, then clear the flag so that it does not fire the update interrupt if enabled
        LL_TIM_GenerateEvent_UPDATE(TIM1);
        LL_TIM_ClearFlag_UPDATE(TIM1);

        // enable software control of outputs
        LL_TIM_EnableAllOutputs(TIM1);
        // enable the timer
        LL_TIM_EnableCounter(TIM1);

    }

    static void disable() {
        LL_TIM_DisableAllOutputs(TIM1);
        LL_TIM_DisableCounter(TIM1);
        LL_APB1_GRP2_DisableClock(LL_APB1_GRP2_PERIPH_TIM1);
    }

    static void enableChannelPWM(uint32_t channel, uint32_t duty) {
        LL_TIM_OC_InitTypeDef TIM_OC_Initstruct ={0};
        TIM_OC_Initstruct.OCMode        = LL_TIM_OCMODE_PWM1;
        TIM_OC_Initstruct.OCState       = LL_TIM_OCSTATE_ENABLE;  
        TIM_OC_Initstruct.OCPolarity    = LL_TIM_OCPOLARITY_HIGH; 
        TIM_OC_Initstruct.OCIdleState   = LL_TIM_OCIDLESTATE_LOW; 
        TIM_OC_Initstruct.CompareValue  = duty;
        LL_TIM_OC_Init(TIM1, 1 << ((channel - 1) * 4), &TIM_OC_Initstruct);
    }

    static void disableChannel(uint32_t channel) {
        LL_TIM_OC_InitTypeDef TIM_OC_Initstruct ={0};
        TIM_OC_Initstruct.OCMode        = LL_TIM_OCMODE_PWM1;     
        TIM_OC_Initstruct.OCState       = LL_TIM_OCSTATE_DISABLE;
        LL_TIM_OC_Init(TIM1, 1 << ((channel - 1) * 4), &TIM_OC_Initstruct);
    }

}; // timer1

/** Timer 3 (general purpose)
 
    This is less advanced than timer1, but still plenty powerful. The API is identical to that of TIM1, at least for the PWM output already implemented.
 */
class timer3 {
public:

    static void enable(uint32_t resolution = 256, uint32_t prescaler = 1) {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

        LL_TIM_SetClockDivision(TIM3, LL_TIM_CLOCKDIVISION_DIV1);
        LL_TIM_SetCounterMode(TIM3, LL_TIM_COUNTERMODE_UP);
        LL_TIM_SetPrescaler(TIM3, prescaler - 1);
        LL_TIM_SetAutoReload(TIM3, resolution - 1);
        LL_TIM_SetRepetitionCounter(TIM3, 0);
        // prescaler is only loaded on update event, force one so that the first period is correct too, then clear the flag so that it does not fire the update interrupt if enabled
        LL_TIM_GenerateEvent_UPDATE(TIM3);
        LL_TIM_ClearFlag_UPDATE(TIM3);

        LL_TIM_EnableAllOutputs(TIM3);
        LL_TIM_EnableCounter(TIM3);
    }

    static void disable() {
        LL_TIM_DisableAllOutputs(TIM3);
        LL_TIM_DisableCounter(TIM3);
        LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_TIM3);
    }

    static void enableChannelPWM(uint32_t channel, uint32_t duty) {
        LL_TIM_OC_InitTypeDef TIM_OC_Initstruct ={0};
        TIM_OC_Initstruct.OCMode        = LL_TIM_OCMODE_PWM1;
        TIM_OC_Initstruct.OCState       = LL_TIM_OCSTATE_ENABLE;  
        TIM_OC_Initstruct.OCPolarity    = LL_TIM_OCPOLARITY_HIGH; 
        TIM_OC_Initstruct.OCIdleState   = LL_TIM_OCIDLESTATE_LOW; 
        TIM_OC_Initstruct.CompareValue  = duty;
        LL_TIM_OC_Init(TIM3, 1 << ((channel - 1) * 4), &TIM_OC_Initstruct);
    }

    static void disableChannel(uint32_t channel) {
        LL_TIM_OC_InitTypeDef TIM_OC_Initstruct ={0};
        TIM_OC_Initstruct.OCMode        = LL_TIM_OCMODE_PWM1;     
        TIM_OC_Initstruct.OCState       = LL_TIM_OCSTATE_DISABLE;
        LL_TIM_OC_Init(TIM3, 1 << ((channel - 1) * 4), &TIM_OC_Initstruct);
    }

}; // timer3

/** Real Time Clock 
 
    The chip has an internal 32kHz clock which can be used with zero external parts, but is extremely imprecise. The external clock needs the crystal and load capacitors, but is actually useful for time keeping and thus should be used by default. 

    The RTC comes with a counter and alarm registers that can be used to keep track of time wake the device up at given time. The counter is a 32bit value incremented every second, so has a range of 136 years. 

    This is how an RTC interrupt handler can look:

    extern "C" {
        void RTC_IRQHandler(void) {
            if (LL_RTC_IsActiveFlag_SEC(RTC)) {
                LL_RTC_ClearFlag_SEC(RTC);
                // code
            }
            if (LL_RTC_IsActiveFlag_ALR(RTC)) {
                LL_RTC_ClearFlag_ALR(RTC);
                // code
            }
        }
    }
*/
class rtc {
public:
    /** Enables the RTC clock. 
   
        Note that the functions spins forever if the external clock was selected, but cannot be started. Internal clock can always be started by definition.

        Calling this function resets the RTC counter so it should only be called once at the beginning of the program.
     */
    static void enable(bool internal = false) {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_RTC);
        LL_PWR_EnableBkUpAccess();
        // reset the backup domain first, LSEON lives in RCC->BDCR and would be cleared by the reset
        LL_RCC_ForceBackupDomainReset();
        LL_RCC_ReleaseBackupDomainReset();
        if (internal) {
            LL_RCC_LSI_Enable();
            while (LL_RCC_LSI_IsReady() != 1);
        } else {
            LL_RCC_LSE_Enable();
            // TODO timeout wait? 
            while (LL_RCC_LSE_IsReady() != 1);
        }
        LL_RCC_SetRTCClockSource(internal ? LL_RCC_RTC_CLKSOURCE_LSI : LL_RCC_RTC_CLKSOURCE_LSE);
        LL_RCC_EnableRTC();
        // initialize the RTC to 1Hz, waits for synchronization internally
        LL_RTC_InitTypeDef init{
            .AsynchPrescaler = 0x7FFF, // 32768 Hz -> 1 Hz
            .OutPutSource = LL_RTC_CALIB_OUTPUT_NONE
        };
        LL_RTC_Init(RTC, &init);
        LL_RTC_TIME_SetCounter(RTC, 0);
    }

    static bool enabled() {
        return LL_RCC_IsEnabledRTC() == 1;
    }

    /** Returns the raw counter value. 
     
        Since the value cannot be read at once, its read in a loop to ensure that the high part does not change while reading the low part. In practice the loop will only run twice at worst.
     */
    static uint32_t counter() {
        while (true) {
            uint32_t high = RTC->CNTH;
            uint32_t low = RTC->CNTL;
            if (high == RTC->CNTH)
                return (high << 16) | (low & 0xffff);
        }
    }

    static void setCounter(uint32_t value) {
        LL_RTC_TIME_SetCounter(RTC, value);
    }

    /** Sets the alarm counter. 
     
        When the counter is identical to that of the alarm counter, the alarm interrupt is triggered. It is not possible to read the alarm counter from the HW. 
     */
    static void setAlarmCounter(uint32_t value) {
        LL_RTC_ALARM_SetCounter(RTC, value);
    }

    /** Enables or disables the second interrupt. 
     */
    static void enableSecondInterrupt(bool value = true) {
        if (value) {
            LL_RTC_EnableIT_SEC(RTC);
            // RTC interrupt is routed to the NVIC through EXTI line 19
            LL_EXTI_EnableIT(LL_EXTI_LINE_19);
            NVIC_SetPriority(RTC_IRQn, 0);
            NVIC_EnableIRQ(RTC_IRQn);
        } else {
            LL_RTC_DisableIT_SEC(RTC);
        }
    }

    /** Enables or disables the alarm interrupt. 
     */
    static void enableAlarmInterrupt(bool value = true) {
        if (value) {
            LL_RTC_EnableIT_ALR(RTC);
            // RTC interrupt is routed to the NVIC through EXTI line 19
            LL_EXTI_EnableIT(LL_EXTI_LINE_19);
            NVIC_SetPriority(RTC_IRQn, 0);
            NVIC_EnableIRQ(RTC_IRQn);
        } else {
            LL_RTC_DisableIT_ALR(RTC);
        }
    }

}; // rtc
