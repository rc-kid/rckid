#ifndef ARCH_RCKID_2
#error "You are building RCKid mk II backend without the indicator macro"
#endif

#include <pico/rand.h>
#include <bsp/board.h>
#include "tusb_config.h"
#include "tusb.h"
#include <hardware/structs/usb.h>

#include <platform/peripherals/bmi160.h>
#include <platform/peripherals/ltr390uv.h>



#include "screen/ST7789.h"

#include "rckid/rckid.h"
#include "rckid/internals.h"

#include "avr/src/state.h"


/** 
    \section rckid_mk2_backend RCKid mk II Backend 
    \addtogroup backends
 
    NOTE: This is a temporary backend that uses the older V2 revision (RP2040 and ATTiny) to allow running the basic SDK on the previous RCKid hardware version. Once the V3 hardware is built and tested, this code will be obsoleted and removed from the repository. 

    The V2 backend is rather complicated because of the small number of RP2040 IO pins required an additional MCU - ATTiny3217 that controls power management and input/output (buttons, LEDs, rumbler). See \ref RP2040Pinout for more details on the hardware connections.
 */

namespace rckid {

    namespace {
        DisplayMode displayMode_ = DisplayMode::Off;
        DisplayUpdateCallback displayCallback_;

        static constexpr unsigned TICK_DONE = 0;
        static constexpr unsigned TICK_ALS = 1;
        static constexpr unsigned TICK_UV = 2;
        static constexpr unsigned TICK_ACCEL = 3;
        static constexpr unsigned TICK_AVR = 4;
        volatile unsigned tickInProgress_ = TICK_DONE;

        platform::BMI160 accelerometer_;
        platform::LTR390UV alsSensor_;
        platform::BMI160::State aState_;
        uint16_t lightAls_ = 0;
        uint16_t lightUV_ = 0;
        State state_; 
        State lastState_;
        uint8_t ticks_ = 0;
        
    }

    void __not_in_flash_func(i2cFillAVRTxBlocks)() {
        i2c0->hw->enable = 0;
        i2c0->hw->tar = I2C_AVR_ADDRESS;
        i2c0->hw->enable = 1;
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_STOP_BITS; // 1 for read, stop
        i2c0->hw->rx_tl = 7;
    }

    void __not_in_flash_func(i2cFillAccelTxBlocks)() {
        i2c0->hw->enable = 0;
        i2c0->hw->tar = accelerometer_.address;
        i2c0->hw->enable = 1;
        i2c0->hw->data_cmd = platform::BMI160::REG_DATA;
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_RESTART_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_STOP_BITS; // 1 for 
        i2c0->hw->rx_tl = 11;
    }

    void __not_in_flash_func(irqI2CDone_)() {
        uint32_t cause = i2c0->hw->intr_stat;
        i2c0->hw->clr_intr;
        if (cause == I2C_IC_INTR_MASK_M_RX_FULL_BITS) {
            switch (tickInProgress_) {
                case TICK_ALS:
                case TICK_UV: {
                    uint16_t value = (i2c0->hw->data_cmd) & 0xff;
                    value += (i2c0->hw->data_cmd & 0xff) * 256;        
                    if (tickInProgress_ == TICK_ALS)
                        lightAls_ = value;
                    else
                        lightUV_ = value;
                    i2cFillAccelTxBlocks();
                    tickInProgress_ = TICK_ACCEL;
                    return;
                }
                case TICK_ACCEL: {
                    // store the accelerometer data
                    uint8_t * raw = reinterpret_cast<uint8_t*>(&aState_);
                    for (int i = 0; i < 12; ++i)
                        *(raw++) = i2c0->hw->data_cmd;
                    // update the accelerometer X and Y axes
                    int16_t ax = - aState_.accelY;
                    int16_t ay = - aState_.accelX;
                    aState_.accelX = ax;
                    aState_.accelY = ay;
                    // and the gyroscope
                    aState_.gyroX *= -1;
                    aState_.gyroY *= -1;
                    // fill in the AVR data
                    i2cFillAVRTxBlocks();
                    tickInProgress_ = TICK_AVR;
                    return;
                }
                case TICK_AVR: {
                    lastState_ = state_;
                    uint8_t * raw = reinterpret_cast<uint8_t*>(&state_);
                    for (int i = 0; i < 8; ++i)
                        *(raw++) = i2c0->hw->data_cmd;
                    // update battery level gauge
                    /*
                    unsigned battPct = vBatt();
                    if (battPct <= VCC_CRITICAL_THRESHOLD)
                        battPct = 0;
                    else if (battPct >= VBATT_FULL_THRESHOLD)
                        battPct = 100;
                    else 
                        battPct = (battPct - VCC_CRITICAL_THRESHOLD) * 100 / (VBATT_FULL_THRESHOLD - VCC_CRITICAL_THRESHOLD);
                    DeviceWrapper::batteryLevel_ = battPct;
                    */
                } // fallthrough to default handler and to disabling the I2C comms
                default:
                    // we are done with the I2C transfer
                    break;
            }
        } else {
            //++stats::i2cErrors_;
        }
        // everything else than tx empty bits terminates the i2c transfer for the current tick
        tickInProgress_ = TICK_DONE;
        i2c0->hw->intr_mask = 0;
        i2c0->hw->enable = 0;
    }



    void __not_in_flash_func(irqDMADone_)() {
        //gpio::outputHigh(GPIO21);
        unsigned irqs = dma_hw->ints0;
        dma_hw->ints0 = irqs;
        // for audio, reset the DMA start address to the beginning of the buffer and tell the stream to refill
//        if (irqs & (1u << audio::dma0_))
//            audio::irqHandler1();
//        if (irqs & (1u << audio::dma1_))
//            audio::irqHandler2();
        // display
        if (irqs & ( 1u << ST7789::dma_))
            ST7789::irqHandler();
        //gpio::outputLow(GPIO21);
    }

    void initialize() {
        board_init();
        /* TODO enable the cartridge uart port based on some preprocessor flag. WHen enabled, make all logs, traces and debugs go to the cartridge port as well. 
        stdio_uart_init_full(
            RP_DEBUG_UART, 
            RP_DEBUG_UART_BAUDRATE, 
            RP_DEBUG_UART_TX_PIN, 
            RP_DEBUG_UART_RX_PIN
        ); */
        // initialize the USB
        tud_init(BOARD_TUD_RHPORT);

        // initialize the I2C bus
        i2c_init(i2c0, RP_I2C_BAUDRATE); 
        i2c0->hw->intr_mask = 0;
        gpio_set_function(RP_PIN_SDA, GPIO_FUNC_I2C);
        gpio_set_function(RP_PIN_SCL, GPIO_FUNC_I2C);
        // Make the I2C pins available to picotool
        bi_decl(bi_2pins_with_func(RP_PIN_SDA, RP_PIN_SCL, GPIO_FUNC_I2C));  

        //usb_hw->main_ctrl = 0;
        // set the single DMA IRQ 0 handler reserved for the SDK
        irq_set_exclusive_handler(DMA_IRQ_0, irqDMADone_);
        irq_set_exclusive_handler(I2C0_IRQ, irqI2CDone_);
        //irq_set_exclusive_handler(TIMER_IRQ_0, irqBSOD_);
        irq_set_enabled(I2C0_IRQ, true);
        // make the I2C IRQ priority larger than that of the DMA (0x80) to ensure that I2C comms do not have to wait for render done if preparing data takes longer than sending them
        irq_set_priority(I2C0_IRQ, 0x40); 
        //irq_set_priority(SPI0_IRQ, 0x40);
        //irq_set_priority(SPI1_IRQ, 0x40);
        // set TIMER_IRQ_0 used for fatal error BSOD to be the highest
        //irq_set_priority(TIMER_IRQ_0, 0x0);

        // initialize the display
        ST7789::initialize();

        // initialize the accelerometer & uv light sensor
        accelerometer_.initialize();
        alsSensor_.initialize();
        alsSensor_.startALS();

        // enter base arena for the application
        memoryEnterArena();
    }

    void tick() {
        while (tickInProgress_ != TICK_DONE)
            yield();
        // make sure the I2C is off, then set it up so that it can talk to the accelerometer
        if (ticks_ % 8 == 0) {
            i2c0->hw->enable = 0;
            i2c0->hw->tar = alsSensor_.address;
            i2c0->hw->enable = 1;
            i2c0->hw->data_cmd = platform::LTR390UV::REG_CTRL; 
            i2c0->hw->data_cmd = ((ticks_ % 16) == 0) ? platform::LTR390UV::CTRL_UV_EN : platform::LTR390UV::CTRL_ALS_EN;
            i2c0->hw->data_cmd = (((ticks_ % 16) == 0) ? platform::LTR390UV::REG_DATA_ALS : platform::LTR390UV::REG_DATA_UV) | I2C_IC_DATA_CMD_RESTART_BITS;
            i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_RESTART_BITS; // 1 for read
            i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_STOP_BITS; // 1 for read
            i2c0->hw->rx_tl = 1;
            tickInProgress_ = (ticks_ % 16 == 0) ? TICK_ALS : TICK_UV; 
        } else {
            i2cFillAccelTxBlocks();
            tickInProgress_ = TICK_ACCEL;
        }
        // make the TX_EMPTY irq fire only when the data is actually processed
        //i2c0->hw->con |= I2C_IC_CON_TX_EMPTY_CTRL_BITS;
        // enable the I2C
        i2c0->hw->intr_mask = I2C_IC_INTR_MASK_M_RX_FULL_BITS | I2C_IC_INTR_MASK_M_TX_ABRT_BITS;


    }

    void yield() {
        tight_loop_contents();
        tud_task();
    }

    void fatalError(uint32_t error, uint32_t line, char const * file) {
        // TODO ensure that there is enough stack for our functions
        // clear all memory arenas to clean up space, this is guarenteed to succeed as the SDK creates memory arena when it finishes initialization    
        while (memoryInsideArena())
            memoryLeaveArena();
        bsod(error, line, file, nullptr);
        // use yield so that we can keep the USB active as well
        // TODO monitor reset, etc 
        while (true)
            yield();
    }

    void fatalError(Error error, uint32_t line, char const * file) {
        fatalError(static_cast<uint32_t>(error), line, file);
    }

    uint32_t uptimeUs() { return time_us_32(); }
    
    uint32_t random() { return get_rand_32(); }

    Writer debugWrite() {
        return Writer{[](char x) {
            tud_cdc_write(& x, 1);
            if (x == '\n')
                tud_cdc_write_flush();            
        }};
    }    

    // io

    bool buttonState(Btn b, State & state) {
        switch (b) {
            case Btn::Up: return state.btnUp();
            case Btn::Down: return state.btnDown();
            case Btn::Left: return state.btnLeft();
            case Btn::Right: return state.btnRight();
            case Btn::A: return state.btnA();
            case Btn::B: return state.btnB();
            case Btn::Select: return state.btnSel();
            case Btn::Start: return state.btnStart();
            case Btn::VolumeUp: return state.btnVolUp();
            case Btn::VolumeDown: return state.btnVolDown();
            case Btn::Home: return state.btnHome();
            default:
                UNREACHABLE;
        }
    }


    bool btnDown(Btn b) {
        return buttonState(b, state_);

    }

    bool btnPressed(Btn b) {
        return buttonState(b, state_) && ! buttonState(b, lastState_);

    }

    bool btnReleased(Btn b) {
        return ! buttonState(b, state_) && buttonState(b, lastState_);
    }

    int16_t accelX() { return 0; }
    int16_t accelY() { return 0; }
    int16_t accelZ() { return 0; }

    int16_t gyroX() { return 0; }
    int16_t gyroY() { return 0; }
    int16_t gyroZ() { return 0; }

    // display

    DisplayMode displayMode() { 
        return ST7789::displayMode();
     }

    void displaySetMode(DisplayMode mode) {
        ST7789::setDisplayMode(mode);
    }

    uint8_t displayBrightness() { 
        // TODO Read from state
        return 128;
    }

    void displaySetBrightness(uint8_t value) {  
        // TODO send I2C command to AVR
    }

    Rect displayUpdateRegion() {    
        return ST7789::updateRegion();
    }

    void displaySetUpdateRegion(Rect region) { 
        ST7789::setUpdateRegion(region);
    }

    bool displayUpdateActive() {
        return ST7789::dmaUpdateInProgress();
    }

    void displayWaitVSync() { 
        ST7789::waitVSync();
    }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels) {
        ST7789::dmaUpdateAsync(pixels, numPixels);
    }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels, DisplayUpdateCallback callback) {
        ST7789::dmaUpdateAsync(pixels, numPixels, callback);
    }

    // audio

    void audioEnable() {

    }

    void audioDisable() {

    }

    bool audioHeadphones() {

    }

    uint8_t audioVolume() {

    }

    void audioSetVolume(uint8_t value) {

    }

    void audioPlay(DoubleBuffer & data, uint32_t bitrate) {

    }

    void audioRecord(DoubleBuffer & data, uint32_t bitrate) {

    }

    void audioPause() {

    }

    void audioStop() {

    }

    // accelerated functions
    #include "rckid/accelerated.inc.h"
    MEM_FILL_8
    MEM_FILL_16
    MEM_FILL_32

}