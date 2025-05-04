#define DEBUG_OLED


#include "platform/platform.h"
#include "platform/peripherals/nrf24l01.h"
#include "platform/peripherals/neopixel.h"
#include "platform/peripherals/ina219.h"
#ifdef DEBUG_OLED
#include "platform/peripherals/ssd1306.h"
#endif

#include "remote/lego_remote.h"
/** Sets the connection timeout interval after which the motors turn off. Expressed in 64ths of a second. 
 */
#define CONNECTION_TIMEOUT 32  

/** In 1/4 of second. 
 */
#define CONNECTION_RESET 120

using namespace platform;
using namespace remote;

template<typename CHANNEL> struct Channel {
    typename CHANNEL::Config config;
    typename CHANNEL::Control control;
}; // Channel

/** LEGO Remote
 
    A remote controller to be used mainly with lego. Supports DC motors at 5 or 9V, servo motors, PWM channels, digital and analog inputs, neopixels and tones. 

                     -- VDD             GND --
           ML1 (WOA) -- (00) PA4   PA3 (16) -- SCK
           MR1 (WOB) -- (01) PA5   PA2 (15) -- MISO
              NRF_CS -- (02) PA6   PA1 (14) -- MOSI
            NRF_RXTX -- (03) PA7   PA0 (17) -- UPDI
    XL1 (WO2*, A0-8) -- (04) PB5   PC3 (13) -- XR1 (WO3*, A1-9)
    XL2 (WO1*, A0-9) -- (05) PB4   PC2 (12) -- XR2 (lcmp0, A1-8) -- use interrupt for waveform
             NRF_IRQ -- (06) PB3   PC1 (11) -- MR2(WOD)
        NEOPIXEL_PIN -- (07) PB2   PC0 (10) -- ML2(WOC)
                 SDA -- (08) PB1   PB0 (09) -- SCL

    Motor Channels

    2 motor channels (ML and MR) are supported. Each channel controls a single DC motor with two PWM pins for CW and CCW operation in 64 speed steps and braking & coasting. TCD is used to drive the motors' PWM signals. The motor channels use 3x4 pin connectors where the connector can select whether the pin will use 5V or 9V. The connectors can be inserted either way and have the following pinout:

        5V   +   -   9V
        SEL GND GND SEL
        9V   -   +   5V

    Configurable Channels

    4 extra channels are provided, XL1, XL2 and XR1 and XR2. The channels use a 2x3 connector that provides 5V, GND and one pin that can be configured as either of:

    - digital input
    - analog input (ADC)
    - digital output
    - analog output (PWM)

    Timers

    - 20ms intervals for the servo control (RTC)
    - 2.5ms interval for the servo control pulse (TCB0)
    - 2 8bit timers in TCA split mode
    - 1 16bit timer in TCB1 




    - RTC is used for delays
    - TCD is used for motors exclusively
    - TCB1 is used for precise servo control timing
    - TCA0 in split mode is used for the PWM channels (2 extra pins left) -- some positioned directly
    - TCB0 for tone generation (so that we can use arduino's tone function by default)

    - maye have the tone generator repurpose customIOs w/o tone stuff in the custom (i.e. mark it as out)

 */
class Remote {
public:

    static constexpr gpio::Pin NEOPIXEL_PIN = 7; 

    static constexpr gpio::Pin ML1_PIN = 0; // TCD, WOA
    static constexpr gpio::Pin MR1_PIN = 1; // TCD, WOB
    static constexpr gpio::Pin ML2_PIN = 10; // TCD, WOC
    static constexpr gpio::Pin MR2_PIN = 11; // TCD, WOD

    static constexpr gpio::Pin L1_PIN = 4; // PB5, ADC0-8, TCA-WO2* (low channel 2)
    static constexpr gpio::Pin L2_PIN = 5; // PB4, ADC0-9, TCA-WO1* (low channel 1)
    static constexpr gpio::Pin R1_PIN = 13; // PC3, ADC1-9, TCA-W03* (high channel 0)
    static constexpr gpio::Pin R2_PIN = 12; // PC2, ADC1-8, uses TCA-W0 (low channel 0) cmp and ovf interrupt to drive the pin

    static constexpr gpio::Pin NRF_CS_PIN = 2;
    static constexpr gpio::Pin NRF_RXTX_PIN = 3;
    static constexpr gpio::Pin NRF_IRQ_PIN = 6;

    static inline uint8_t rtcTicks_ = 0;

    static void initialize() {
        // set CLK_PER prescaler to 2, i.e. 10Mhz, which is the maximum the chip supports at voltages as low as 3.3V
        CCP = CCP_IOREG_gc;
        CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm; 
        gpio::initialize();

        // clear all RGB colors, set the control LED to green & update
        gpio::output(NEOPIXEL_PIN);
        rgbColors_.clear();
        rgbColors_[0] = Color::White();
        rgbColors_.update();

        i2c::initializeMaster();
        spi::initialize();

        // set configurable channel pins to input 
        gpio::input(L1_PIN);
        gpio::input(L2_PIN);
        gpio::input(R1_PIN);
        gpio::input(R2_PIN);
        // ensure motor pins output low so that any connected motors are floating
        gpio::output(ML1_PIN);
        gpio::low(ML1_PIN);
        gpio::output(ML2_PIN);
        gpio::low(ML2_PIN);
        gpio::output(MR1_PIN);
        gpio::low(MR1_PIN);
        gpio::output(MR2_PIN);
        gpio::low(MR2_PIN);

        initializeMotorControl();
        initializeServoControl();
        initializeAnalogInputs();
        initializePWMOutputs();
        initializeAudio();
        initializePowerManagement();
#ifdef DEBUG_OLED
        initializeDebug();
#endif

        // initialize the effects timer of 64Hz (15.6ms) for RGB & tone switches
        RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
        RTC.PITCTRLA = RTC_PERIOD_CYC32_gc | RTC_PITEN_bm;
        // TODO also use this to ensure that we have a connection with the controller and turn off motors & stuff if we don't 

        

        // initialize the NRF radio
        if (!initializeRadio()) {
            rgbColors_[0] = Color::Red();
            rgbColors_.update();
            while (true) {}
        }

        // turn the white color off to signify we are done with startup
        rgbColors_.clear();
        rgbColors_.update();

        // TODO delete this when ready
        setChannelConfig(LegoRemote::CHANNEL_L1, (uint8_t*) & channel::CustomIO::Config::output());
        setChannelConfig(LegoRemote::CHANNEL_L2, (uint8_t*) & channel::CustomIO::Config::inputPullup());
        setChannelConfig(LegoRemote::CHANNEL_TONE_EFFECT, (uint8_t*) & channel::ToneEffect::Config{LegoRemote::CHANNEL_L1});
        startToneEffect(& channel::ToneEffect::Control::yelp(600, 1200, 1500));
    }

    static void loop() {
        checkDigitalIn();
        checkAnalogIn();
        servoTick();
        if (gpio::read(NRF_IRQ_PIN) == 0)
            radioIrq();
        if (RTC.PITINTFLAGS == RTC_PI_bm) {
            RTC.PITINTFLAGS = RTC_PI_bm;
            ++rtcTicks_;
            // send the delayed response
            if (responseTimeout_ > 0 && --responseTimeout_ == 0) 
                radioResponse();
            if (rtcTicks_ % 16 == 0) { // 64Hz fo effects, power and connection timeouts
                // check the power settings 
                checkPower();
                if (connTimeout_ > 0) {
                    if (connected()) {
                        if (rtcTicks_ % 10 == 0 && --connTimeout_ == 0)
                            connectionLost();
                    } else {
                        if (rtcTicks_ == 0 && --connTimeout_ == 0)
                            cpu::reset();
                    }
                }
                // do tone effect if any
                toneEffectTick();
                // do rgb effect, if any 
                rgbEffectTick();
            }
            
#ifdef DEBUG_OLED
            if (rtcTicks_ == 0 && (++dbgTimeout_ % 4) ==0) // 1Hz
                debugPrint();
#endif
        }
   
    }

#ifdef DEBUG_OLED
    /** \name Debug I2C display 
     
        Displays the messages 
     */
    //@{
    static inline SSD1306 oled_;
    static inline uint8_t dbgTimeout_ = 0;
    static inline uint16_t rx_ = 0;
    static inline uint16_t tx_ = 0;
    static inline uint16_t errors_ = 0;

    static void initializeDebug() {
        oled_.initialize128x32();
        oled_.normalMode();
        oled_.clear32();
        oled_.write(0, 0, "RX:");
        oled_.write(64, 0, "TX:");

        oled_.write(0, 1, "ML:");
        oled_.write(0, 2, "L1:");
        oled_.write(0, 3, "L2:");
        oled_.write(64, 1, "MR:");
        oled_.write(64, 2, "R1:");
        oled_.write(64, 3, "R2:");
    }

    /* */
    static void debugPrint() {
        if (paired()) {
            debugMotor(20, 1, ml_);
            debugMotor(84, 1, mr_);
            debugCustomIOChannel(20, 2, l1_.config, l1_.control, feedback_.l1);
            debugCustomIOChannel(20, 3, l2_.config, l2_.control, feedback_.l2);
            debugCustomIOChannel(84, 2, r1_.config, r1_.control, feedback_.r1);
            debugCustomIOChannel(84, 3, r2_.config, r2_.control, feedback_.r2);
            oled_.write(20, 0, rx_);
            oled_.write(84, 0, tx_);
        } else {
            char x[6] = { 0,0,0,0,0,0};
            radio_.txAddress(x);
            oled_.write(0, 1, x);
            radio_.rxAddress(x);
            oled_.write(0, 2, x);
            oled_.write(20, 0, rx_);
            oled_.write(84, 0, tx_);
        }
    }

    static void debugMotor(uint8_t x, uint8_t y, Channel<channel::Motor> & m) {
        switch (m.control.mode) {
            case channel::Motor::Mode::Coast:
                oled_.write(x, y, "coast ");
                break;
            case channel::Motor::Mode::Brake:
                oled_.write(x, y, "brake ");
                break;
            case channel::Motor::Mode::CW:
                oled_.write(x, y, "cw     ");
                oled_.write(x + 20, y, m.control.speed);
                break;
            case channel::Motor::Mode::CCW:
                oled_.write(x, y, "ccw    ");
                oled_.write(x + 20, y, m.control.speed);
                break;
        }
    }

    static void debugCustomIOChannel(uint8_t x, uint8_t y, channel::CustomIO::Config & config, channel::CustomIO::Control & control, channel::CustomIO::Feedback & feedback) {
        switch (config.mode) {
            case channel::CustomIO::Mode::DigitalIn:
               oled_.write(x, y, "di     ");
               oled_.write(x + 20, y, feedback.value);
               break;
            case channel::CustomIO::Mode::DigitalOut:
               oled_.write(x, y, "do     ");
               oled_.write(x + 20, y, control.value);
               break;
            case channel::CustomIO::Mode::AnalogIn:
               oled_.write(x, y, "ai     ");
               oled_.write(x + 20, y, feedback.value);
               break;
            case channel::CustomIO::Mode::PWM:
               oled_.write(x, y, "pwm     ");
               oled_.write(x + 20, y, control.value);
               break;
            case channel::CustomIO::Mode::Servo:
               oled_.write(x, y, "pwm     ");
               oled_.write(x + 20, y, control.value);
               break;
        }
    }

    //@}
#endif

    /** \name Power Management 
     */
    //@{

    static inline INA219 iSenseTotal_{0x40};
    static inline INA219 iSenseML_{0x44};
    static inline INA219 iSenseMR_{0x45};

    static void initializePowerManagement() {
        // 8mOhm for 10Amps max resolution in mA
        iSenseTotal_.initialize(INA219::Gain::mv_80, 8);
        /*
        // for the motors, we take 4mOhms for 5Amps max resolution
        iSenseML_.initialize(INA219::Gain::mv_80, 4);
        iSenseMR_.initialize(INA219::Gain::mv_80, 4);
        */
    }

    static void checkPower() {
        uint16_t iTotal = iSenseTotal_.current();
        /*
        uint16_t iML = iSenseML_.current();
        uint16_t iMR = iSenseMR_.current();
        */
    }

    static void activate() {

    }

    static void deactivate() {
        //feedback_.device.setActive(false);
        // TODO do stuff fere
        //setMotorL(channel::Motor::Control::Coast());
        //setMotorR(channel::Motor::Control::Coast());
        // TODO turn off the rest

    }

    //@}

    /** \name Radio comms
     
        
     */
    //@{
    static inline NRF24L01 radio_{NRF_CS_PIN, NRF_RXTX_PIN};
    static inline bool transmitting_ = false;
    static inline uint8_t rxBuffer_[32];
    static inline uint8_t txBuffer_[32];
    static inline uint8_t errorBuffer_[32];
    static inline uint8_t errorIndex_; 
    static inline uint8_t * response_;
    static inline uint8_t responseTimeout_ = 0;
    static inline uint16_t connTimeout_ = 0;
    static inline uint8_t controller_[] = { 0, 0, 0, 0, 0};
    static inline uint16_t deviceId_;
    static inline char deviceName_[16];

    /** Initializes the radio and enters the receiver mode.
     */
    static bool initializeRadio() {
        if (!radio_.initialize(msg::DefaultAddress, msg::DefaultAddress, msg::DefaultChannel))
            return false;
        radio_.standby();
        radio_.enableReceiver();
        errorBuffer_[0] = msg::Error::ID;
        deviceId_ = 12345;
        memcpy(deviceName_, "LegoRemote", 11);
        return true;
    }

    static bool paired() { return controller_[0] != 0; }

    static bool connected() { return controller_[0] != 0 && ! feedback_.device.connectionLost(); }

    static bool isPairedController(uint8_t const * name) {
        if (controller_[0] == 0)
            return false;
        for (uint8_t i = 0; i < 5; ++i)
            if (controller_[i] != name[i])
                return false;
        return true;
    }

    static void error(msg::ErrorKind err, uint8_t data = 0) {
        errorBuffer_[errorIndex_++] = static_cast<uint8_t>(err);
        errorBuffer_[errorIndex_++] = data;
        // tentatively write the end of error info after this one 
        if (errorIndex_ < 32)
            errorBuffer_[errorIndex_] = 0; 
    } 

    /** Receive command, process it and optionally send a reply.
     */
    static void radioIrq() {
        
        if (transmitting_) {
            radio_.clearIrq();
            NRF24L01::FifoStatus fifo = radio_.getFifoStatus();
            if (fifo.txEmpty()) {
                radio_.standby();
                radio_.enableReceiver();
                transmitting_ = false;
                // if the rx fifo is empty, we can quit immediately and will be notified of the next message by the IRQ, otherwise if there is something continue to the receive check and message processing to ensure the message will be processes (the IRQ is lost by the above clear action)
                if (fifo.rxEmpty())
                    return;
            } else {
                return;
            }
        } else {
            radio_.clearDataReadyIrq();
        }
        // try processing the message, if any
        if  (radio_.receive(rxBuffer_, 32)) {
#if (defined DEBUG_OLED)
            ++rx_;
#endif
            // set the response to txBuffer and load with NOP message (which we won't transmit if not changed)
            txBuffer_[0] = msg::Nop::ID;
            response_ = txBuffer_;
            // reset the error size so that we know whether to send it or not
            errorIndex_ = 1;
            if (!paired() && rxBuffer_[0] > msg::Pair::ID) {
                error(msg::ErrorKind::DeviceNotPaired, rxBuffer_[0]);
            } else {
                switch (rxBuffer_[0]) {
                    // set the tx address to the one provided in the request and then send the information 
                    case msg::RequestDeviceInfo::ID: {
                        // TODO add a random delay(?)
                        cpu::delayMs(100);
                        uint8_t * name = reinterpret_cast<msg::RequestDeviceInfo const *>(rxBuffer_)->controllerAddress;
                        if (!connected() || isPairedController(name)) {
                            radio_.setTxAddress(name);
                            new (txBuffer_) msg::DeviceInfo{LegoRemote::NUM_CHANNELS, deviceId_, deviceName_};
                        } 
                        break;
                    }
                    // reset the rx and tx addresses to default and clear the pairing flag
                    case msg::Reset::ID: {
                        uint8_t * name = reinterpret_cast<msg::Reset const *>(rxBuffer_)->controllerAddress;
                        if (!paired() || isPairedController(name)) {
                            radio_.initialize(msg::DefaultAddress, msg::DefaultAddress, msg::DefaultChannel);
                            radio_.standby();
                            radio_.enableReceiver();
                            controller_[0] = 0; // clear pairing info
                        }
                        break;
                    }
                    // pairs the device with the controller, sets the rx and tx addresses accordingly and updates the channel as requested 
                    case msg::Pair::ID: {
                        msg::Pair const * msg = reinterpret_cast<msg::Pair const *>(rxBuffer_);
                        if (msg->deviceId == deviceId_ && (strncmp(msg->deviceName, deviceName_, 15) == 0)) {
                            if (!connected() || isPairedController(msg->controllerAddress)) {
                                radio_.initialize(msg->deviceAddress, msg->controllerAddress, msg->channel);
                                radio_.standby();
                                radio_.enableReceiver();
                                // and enter the paired mode
                                for (uint8_t i = 0; i < 5; ++i)
                                    controller_[i] = msg->controllerAddress[i];
                            }
                        } 
                        // leave response at NOP
                        break;
                    }
                    // returns the channel information. This is a bit silly as we do not store the whole 32 bytes in the channel info, but we don't care if we send garbage after the final 0
                    case msg::GetChannelInfo::ID:
                        response_ = LegoRemote::CHANNEL_INFO;
                        break;
                    // returns the channel configuration for the provided channel number
                    case msg::GetChannelConfig::ID:
                        getChannelConfig(reinterpret_cast<msg::GetChannelConfig const *>(rxBuffer_)->channel);
                        break;
                    // sets the channel config for given channel    
                    case msg::SetChannelConfig::ID: {
                        msg::SetChannelConfig const * msg = reinterpret_cast<msg::SetChannelConfig const *>(rxBuffer_);
                        setChannelConfig(msg->channel, msg->config);
                        response_ = reinterpret_cast<uint8_t const *>(&feedback_);
                        break;
                    }
                    // returns the channel control for the provided channel number
                    case msg::GetChannelControl::ID:
                        getChannelControl(reinterpret_cast<msg::GetChannelControl const *>(rxBuffer_)->channel);
                        break;
                    // sets channel control for specific channels
                    case msg::SetChannelControl::ID: {
                        uint8_t i = 1;
                        while (rxBuffer_[i] != 0) {
                            uint8_t channelIndex = rxBuffer_[i++];
                            i += setChannelControl(channelIndex, rxBuffer_ + i);

                        }
                        break;
                    }
                    case msg::SetControlConsecutive::ID: {
                        msg::SetControlConsecutive const * msg = reinterpret_cast<msg::SetControlConsecutive const *>(rxBuffer_);
                        uint8_t i = 0;
                        for (uint8_t channel = msg->fromChannel, echannel = msg->fromChannel + msg->numChannels; ++channel; channel != echannel)
                           i += setChannelControl(channel, msg->data + i);
                        break; 
                    }

                    // returns feedback for the given channel
                    case msg::GetChannelFeedback::ID: 
                        getChannelFeedback(reinterpret_cast<msg::GetChannelFeedback const *>(rxBuffer_)->channel);
                        break;
                    default:    
                        error(msg::ErrorKind::InvalidCommand, rxBuffer_[0]);
                        break;
                }
                if (errorIndex_ > 1 || response_[0] != msg::Nop::ID)
                    responseTimeout_ = 7;
            }
            // reset the connection timeout
            if (connected())
                connTimeout_ = CONNECTION_TIMEOUT;
        }
    }

    static void radioResponse() {
        // send the optional error and feedback back to keep the comms
        if (errorIndex_ > 1 || response_[0] != msg::Nop::ID) {
            radio_.standby();
            if (errorIndex_ > 1) {
                radio_.transmit(errorBuffer_, 32);
#if (defined DEBUG_OLED)
                ++tx_;
#endif
            }
            if (response_[0] != msg::Nop::ID) {
                radio_.transmit(response_, 32);
#if (defined DEBUG_OLED)
                ++tx_;
#endif
            }
            transmitting_ = true;
            radio_.enableTransmitter();
        }
    }

    /** Loss of connection event. 
     */
    static void connectionLost() {
        deactivate();
        feedback_.device.setConnectionLost();
        connTimeout_ = CONNECTION_RESET;
    }

    //@}

    /** \name Channels
     
     */
    //@{


    static inline Channel<channel::Motor> ml_;
    static inline Channel<channel::Motor> mr_;
    static inline Channel<channel::CustomIO> l1_;
    static inline Channel<channel::CustomIO> r1_;
    static inline Channel<channel::CustomIO> l2_;
    static inline Channel<channel::CustomIO> r2_;
    static inline Channel<channel::ToneEffect> tone_;
    static inline Channel<channel::RGBStrip> rgb_;

    /** Feedback for all channels. 
     */
    static inline LegoRemote::Feedback feedback_;

    template<typename T>
    static void getChannelDataFrom(channel::Kind kind, T const & config) {
        txBuffer_[1] = static_cast<uint8_t>(kind);
        memcpy(txBuffer_ + 2, & config, sizeof(T));
    }

    /** Gets configuration for given channel */
    static void getChannelConfig(uint8_t channel) {
        if (channel < 1 || channel > LegoRemote::NUM_CHANNELS) {
            error(msg::ErrorKind::InvalidChannel);
        } else {
            txBuffer_[0] = msg::ChannelConfig::ID;
            switch (channel) {
                case LegoRemote::CHANNEL_DEVICE: {
                    channel::Device::Config cfg;
                    getChannelDataFrom(channel::Kind::Device, &cfg);
                    break;
                }
                case LegoRemote::CHANNEL_ML:
                    getChannelDataFrom(channel::Kind::Motor, ml_.config);
                    break;
                case LegoRemote::CHANNEL_MR:
                    getChannelDataFrom(channel::Kind::Motor, mr_.config);
                    break;
                case LegoRemote::CHANNEL_L1:
                    getChannelDataFrom(channel::Kind::CustomIO, l1_.config);
                    break;
                case LegoRemote::CHANNEL_R1:
                    getChannelDataFrom(channel::Kind::CustomIO, r1_.config);
                    break;
                case LegoRemote::CHANNEL_L2:
                    getChannelDataFrom(channel::Kind::CustomIO, l2_.config);
                    break;
                case LegoRemote::CHANNEL_R2:
                    getChannelDataFrom(channel::Kind::CustomIO, r2_.config);
                    break;
                case LegoRemote::CHANNEL_TONE_EFFECT:
                    getChannelDataFrom(channel::Kind::ToneEffect, tone_.config);
                    break;
                case LegoRemote::CHANNEL_RGB_STRIP:
                    getChannelDataFrom(channel::Kind::RGBStrip, rgb_.config);
                    break;
                default:
                    txBuffer_[1] = static_cast<uint8_t>(channel::Kind::RGBColor);
                    // no config (?)
                    break;
            }
        }
    }

    static void setChannelConfig(uint8_t channel, uint8_t * data) {
        switch (channel) {
            case LegoRemote::CHANNEL_DEVICE:
                error(msg::ErrorKind::InvalidChannel);
                break;
            case LegoRemote::CHANNEL_ML: 
                ml_.config = *reinterpret_cast<channel::Motor::Config*>(data);
                break;
            case LegoRemote::CHANNEL_MR: 
                mr_.config = *reinterpret_cast<channel::Motor::Config*>(data);
                break;
            case LegoRemote::CHANNEL_L1:
                setCustomIOConfig(l1_, L1_PIN, reinterpret_cast<channel::CustomIO::Config*>(data));
                break;
            case LegoRemote::CHANNEL_R1: 
                setCustomIOConfig(r1_, R1_PIN, reinterpret_cast<channel::CustomIO::Config*>(data));
                break;
            case LegoRemote::CHANNEL_L2: 
                setCustomIOConfig(l2_, L2_PIN, reinterpret_cast<channel::CustomIO::Config*>(data));
                break;
            case LegoRemote::CHANNEL_R2:
                setCustomIOConfig(r2_, R2_PIN, reinterpret_cast<channel::CustomIO::Config*>(data));
                break;
            case LegoRemote::CHANNEL_TONE_EFFECT: 
                tone_.config = *reinterpret_cast<channel::ToneEffect::Config*>(data);
                // TODO check that the output channel is output and error otherwise
                break;
            case LegoRemote::CHANNEL_RGB_STRIP: 
            default:
                error(msg::ErrorKind::InvalidChannel, channel);
                break;
        }
    }

    /** Gets configuration for given channel */
    static void getChannelControl(uint8_t channel) {
        if (channel < 1 || channel > LegoRemote::NUM_CHANNELS) {
            error(msg::ErrorKind::InvalidChannel);
        } else {
            txBuffer_[0] = msg::ChannelControl::ID;
            switch (channel) {
                case LegoRemote::CHANNEL_DEVICE: {
                    channel::Device::Control ctrl{feedback_.device.active()};
                    getChannelDataFrom(channel::Kind::Motor, & ctrl);
                    break;
                }
                case LegoRemote::CHANNEL_ML:
                    getChannelDataFrom(channel::Kind::Motor, ml_.control);
                    break;
                case LegoRemote::CHANNEL_MR:
                    getChannelDataFrom(channel::Kind::Motor, mr_.control);
                    break;
                case LegoRemote::CHANNEL_L1:
                    getChannelDataFrom(channel::Kind::CustomIO, l1_.control);
                    break;
                case LegoRemote::CHANNEL_R1:
                    getChannelDataFrom(channel::Kind::CustomIO, r1_.control);
                    break;
                case LegoRemote::CHANNEL_L2:
                    getChannelDataFrom(channel::Kind::CustomIO, l2_.control);
                    break;
                case LegoRemote::CHANNEL_R2:
                    getChannelDataFrom(channel::Kind::CustomIO, r2_.control);
                    break;
                case LegoRemote::CHANNEL_TONE_EFFECT:
                    getChannelDataFrom(channel::Kind::ToneEffect, tone_.control);
                    break;
                case LegoRemote::CHANNEL_RGB_STRIP:
                    getChannelDataFrom(channel::Kind::RGBStrip, rgb_.control);
                    break;
                default:
                    // TODO
                    break;
            }
        }
    }

    /** Sets channel control for the given channel indes and returns the length of bytes read. 
     */
    static uint8_t setChannelControl(uint8_t channel, uint8_t * data) {
        switch (channel) {
            case LegoRemote::CHANNEL_DEVICE: {
                channel::Device::Control * ctrl = reinterpret_cast<channel::Device::Control*>(data);     
                ctrl->active == true ? activate() : deactivate();
                break;
            }
            case LegoRemote::CHANNEL_ML:
                setMotorL(*reinterpret_cast<channel::Motor::Control*>(data));
                return sizeof(channel::Motor::Control);
            case LegoRemote::CHANNEL_MR:
                setMotorR(*reinterpret_cast<channel::Motor::Control*>(data));
                return sizeof(channel::Motor::Control);
            case LegoRemote::CHANNEL_L1:
                setCustomIOControl(l1_, L1_PIN, reinterpret_cast<channel::CustomIO::Control*>(data));
                return sizeof(channel::CustomIO::Control);
            case LegoRemote::CHANNEL_L2:
                setCustomIOControl(l2_, L2_PIN, reinterpret_cast<channel::CustomIO::Control*>(data));
                return sizeof(channel::CustomIO::Control);
            case LegoRemote::CHANNEL_R1:
                setCustomIOControl(r1_, R1_PIN, reinterpret_cast<channel::CustomIO::Control*>(data));
                return sizeof(channel::CustomIO::Control);
            case LegoRemote::CHANNEL_R2:
                setCustomIOControl(r2_, R2_PIN, reinterpret_cast<channel::CustomIO::Control*>(data));
                return sizeof(channel::CustomIO::Control);
            case LegoRemote::CHANNEL_TONE_EFFECT:
                startToneEffect(reinterpret_cast<channel::ToneEffect::Control*>(data));
                return sizeof(channel::ToneEffect::Control);
            default:
                error(msg::ErrorKind::InvalidChannel, channel);
                // return 32 which will make processing any further channels that might have been in the message impossible as it overflows the rxBuffer  
                return 32; 
        }
    }

    /** Gets configuration for given channel */
    static void getChannelFeedback(uint8_t channel) {
        if (channel < 1 || channel > LegoRemote::NUM_CHANNELS) {
            error(msg::ErrorKind::InvalidChannel);
        } else {
            txBuffer_[0] = msg::ChannelFeedback::ID;
            switch (channel) {
                case LegoRemote::CHANNEL_DEVICE:
                    getChannelDataFrom(channel::Kind::Motor, feedback_.device);
                    break;
                case LegoRemote::CHANNEL_ML:
                    getChannelDataFrom(channel::Kind::Motor, feedback_.ml);
                    break;
                case LegoRemote::CHANNEL_MR:
                    getChannelDataFrom(channel::Kind::Motor, feedback_.mr);
                    break;
                case LegoRemote::CHANNEL_L1:
                    getChannelDataFrom(channel::Kind::CustomIO, feedback_.l1);
                    break;
                case LegoRemote::CHANNEL_R1:
                    getChannelDataFrom(channel::Kind::CustomIO, feedback_.r1);
                    break;
                case LegoRemote::CHANNEL_L2:
                    getChannelDataFrom(channel::Kind::CustomIO, feedback_.l2);
                    break;
                case LegoRemote::CHANNEL_R2:
                    getChannelDataFrom(channel::Kind::CustomIO, feedback_.r2);
                    break;
                case LegoRemote::CHANNEL_TONE_EFFECT:
                    getChannelDataFrom(channel::Kind::ToneEffect, feedback_.tone);
                    break;
                case LegoRemote::CHANNEL_RGB_STRIP:
                    getChannelDataFrom(channel::Kind::RGBStrip, feedback_.rgb);
                    break;
                default:
                    txBuffer_[1] = static_cast<uint8_t>(channel::Kind::RGBColor);
                    // no feedback
                    break;
            }
        }
    }

    static void setCustomIOConfig(Channel<channel::CustomIO> & channel, gpio::Pin pin, channel::CustomIO::Config const * config) {
        disablePWM(pin);
        gpio::input(pin);
        channel.config = *config;
        switch (channel.config.mode) {
            case channel::CustomIO::Mode::DigitalIn:
                if (channel.config.pullup)
                    gpio::inputPullup(pin);
                break;
            case channel::CustomIO::Mode::DigitalOut:
                gpio::output(pin);
                gpio::write(pin, channel.control.value != 0);
                break;
            case channel::CustomIO::Mode::AnalogIn:
                // TODO disable digital buffers
                break; 
            case channel::CustomIO::Mode::PWM:
                setPWM(pin, channel.control.value & 0xff);
                break;
            case channel::CustomIO::Mode::Servo:
                gpio::output(pin);
                gpio::low(pin);
                break;
            default:
                // TODO error & set to digital input
                break;
        }
    }

    static void setCustomIOControl(Channel<channel::CustomIO> & channel, gpio::Pin pin, channel::CustomIO::Control const * control) {
        channel.control = *control;
        switch (channel.config.mode) {
            case channel::CustomIO::Mode::DigitalOut:
                gpio::write(pin, channel.control.value != 0);
                break;
            case channel::CustomIO::Mode::PWM:
                setPWM(pin, channel.control.value & 0xff);
                break;
            default:
                // nothing to do for Digital & analog In and servo 
                break;
        }
    }

    static void checkDigitalIn() {
        if (l1_.config.mode == channel::CustomIO::Mode::DigitalIn)
            feedback_.l1.value = gpio::read(L1_PIN);
        if (l2_.config.mode == channel::CustomIO::Mode::DigitalIn) 
            feedback_.l2.value = gpio::read(L2_PIN);
        if (r1_.config.mode == channel::CustomIO::Mode::DigitalIn) 
            feedback_.r1.value = gpio::read(R1_PIN);
        if (r2_.config.mode == channel::CustomIO::Mode::DigitalIn) 
            feedback_.r2.value = gpio::read(R2_PIN);
    }

    //@}

    /** \name Motor Control

        To engage break, disconnect the timer from both pins and set both to high. To let the motor coast, disconnect the timer from both pins and set both to low. The forward / backward operation is enabled by directing the PWM output to one of the pins only.

     */
    //{@

    static void initializeMotorControl() {
        // initialize TCD used to control the two motors, disable prescalers, set one ramp waveform and set WOC to WOA and WOD to WOB. 
        TCD0.CTRLA = TCD_CLKSEL_20MHZ_gc | TCD_CNTPRES_DIV1_gc | TCD_SYNCPRES_DIV1_gc;
        TCD0.CTRLB = TCD_WGMODE_ONERAMP_gc;
        TCD0.CTRLC = TCD_CMPCSEL_PWMA_gc | TCD_CMPDSEL_PWMB_gc;
        // disconnect the pins from the timer
        CPU_CCP = CCP_IOREG_gc;
        TCD0.FAULTCTRL = 0;
        // enable the timer
        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
        TCD0.CTRLA |= TCD_ENABLE_bm;

        // TODO what is the below???????

        // set the reset counter to 255 for both A and B. This gives us 78.4kHz PWM frequency. It is important for both values to be the same. By setting max to 255 we can simply set 
        while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
        TCD0.CMPACLR = 127;
        while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
        TCD0.CMPBCLR = 127;        
    }

    static bool isSpinning(Channel<channel::Motor> & c) {
        return (c.control.mode == channel::Motor::Mode::CW) || (c.control.mode == channel::Motor::Mode::CCW);
    }

    static void setMotorL(channel::Motor::Control const & ctrl) {
        if (ctrl != ml_.control) {
            switch (ctrl.mode) {
                case channel::Motor::Mode::Brake:
                    if (isSpinning(ml_)) {
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA &= ~TCD_ENABLE_bm;
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL &= ~(TCD_CMPAEN_bm | TCD_CMPCEN_bm);
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA |= TCD_ENABLE_bm;
                    };
                    gpio::high(ML1_PIN);
                    gpio::high(ML2_PIN);
                    break;
                case channel::Motor::Mode::Coast:
                    if (isSpinning(ml_)) {
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA &= ~TCD_ENABLE_bm;
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL &= ~(TCD_CMPAEN_bm | TCD_CMPCEN_bm);
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA |= TCD_ENABLE_bm;
                    }
                    gpio::low(ML1_PIN);
                    gpio::low(ML2_PIN);
                    break;
                case channel::Motor::Mode::CW:
                    while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
                    TCD0.CMPASET = 255 - ctrl.speed;
                    if (ml_.control.mode != channel::Motor::Mode::CW) {
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA &= ~TCD_ENABLE_bm;
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL &= ~(TCD_CMPAEN_bm | TCD_CMPCEN_bm);
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL |= TCD_CMPAEN_bm;
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA |= TCD_ENABLE_bm;
                    }
                    break;
                case channel::Motor::Mode::CCW:
                    while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
                    TCD0.CMPASET = 255 - ctrl.speed;
                    if (ml_.control.mode != channel::Motor::Mode::CCW) {
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA &= ~TCD_ENABLE_bm;
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL &= ~(TCD_CMPAEN_bm | TCD_CMPCEN_bm);
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL |= TCD_CMPCEN_bm;
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA |= TCD_ENABLE_bm;
                    }
                    break;
            }
            ml_.control = ctrl;
        }
    }

    static void setMotorR(channel::Motor::Control const & ctrl) {
        if (ctrl != mr_.control) {
            switch (ctrl.mode) {
                case channel::Motor::Mode::Brake:
                    if (isSpinning(mr_)) {
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA &= ~TCD_ENABLE_bm;
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL &= ~(TCD_CMPBEN_bm | TCD_CMPDEN_bm);
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA |= TCD_ENABLE_bm;
                    }
                    gpio::high(MR1_PIN);
                    gpio::high(MR2_PIN);
                    break;
                case channel::Motor::Mode::Coast:
                    if (isSpinning(mr_)) {
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA &= ~TCD_ENABLE_bm;
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL &= ~(TCD_CMPBEN_bm | TCD_CMPDEN_bm);
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA |= TCD_ENABLE_bm;
                    }
                    gpio::low(MR1_PIN);
                    gpio::low(MR2_PIN);
                    break;
                case channel::Motor::Mode::CW:
                    while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
                    TCD0.CMPBSET = 255 - ctrl.speed;
                    if (mr_.control.mode != channel::Motor::Mode::CW) {
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA &= ~TCD_ENABLE_bm;
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL &= ~(TCD_CMPBEN_bm | TCD_CMPDEN_bm);
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL |= TCD_CMPBEN_bm;
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA |= TCD_ENABLE_bm;
                    }
                    break;
                case channel::Motor::Mode::CCW:
                    while (TCD0.STATUS & TCD_CMDRDY_bm == 0) {};
                    TCD0.CMPBSET = 255 - ctrl.speed;
                    if (mr_.control.mode != channel::Motor::Mode::CCW) {
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA &= ~TCD_ENABLE_bm;
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL &= ~(TCD_CMPBEN_bm | TCD_CMPDEN_bm);
                        CPU_CCP = CCP_IOREG_gc;
                        TCD0.FAULTCTRL |= TCD_CMPDEN_bm;
                        while (TCD0.STATUS & TCD_ENRDY_bm == 0) {};
                        TCD0.CTRLA |= TCD_ENABLE_bm;
                    }
                    break;
            }
            mr_.control = ctrl;
        }

    }

    //@}
    

    /** \name Servo Control 
     
        To be able to drive four servos with single timer (TCB1) we use the RTC overflow timer set to 5ms to multiplex the possibly four times so that each servo motor will get its pulse once per 20ms window as requested by the servo control protocol. 

        The servo tick happens every time the RTC overflows and cycles through the 4 custom IO channels. If the current channel is a servo control, then the TCB1 duration is set according to its pulse and the timer is started while the channel pin is driven high to start the pulse. The interrupt attached to the TCB1 simply turns the timer off and pulls the pin channel pin low.  

        TODO figure out decent values for the servos

        TODO check servos work as intended
     */
    //@{
    static inline uint8_t activeServoPin_;
    static inline uint8_t servoTick_ = 0;

    static void initializeServoControl() {
        // initialize the RTC to fire every 5ms which gives us a tick that can be used to switch the servo controls, 4 servos max, multiplexed gives the freuency of updates for each at 20ms
        RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
        while (RTC.STATUS & RTC_PERBUSY_bm);
        RTC.PER = 164;
        while (RTC.STATUS & RTC_CTRLABUSY_bm);
        RTC.CTRLA = RTC_RTCEN_bm;
        // initialize TCB0 which is used to time the servo control interval precisely
        TCB1.CTRLB = TCB_CNTMODE_INT_gc;
        TCB1.INTCTRL = TCB_CAPT_bm;
        TCB1.CTRLA = TCB_CLKSEL_CLKDIV1_gc; // | TCB_ENABLE_bm;
    }

    static void servoTick() {
        if (! (RTC.INTFLAGS & RTC_OVF_bm))
            return; // no tick
        RTC.INTFLAGS = RTC_OVF_bm;
        servoTick_ = (servoTick_ + 1) % 4;
        switch (servoTick_) {
            case 0: 
                startControlPulse(l1_, L1_PIN);
                break;
            case 1:
                startControlPulse(l2_, L2_PIN);
                break;
            case 2:
                startControlPulse(r1_, R1_PIN);
                break;
            case 3:
                startControlPulse(r1_, R1_PIN);
                break;
        }
    }

    static void startControlPulse(Channel<channel::CustomIO> const & ch, gpio::Pin pin) {
        if (ch.config.mode != channel::CustomIO::Mode::Servo)
            return;
        activeServoPin_= pin;
        // calculate the pulse duration from the config
        TCB1.CTRLA &= ~TCB_ENABLE_bm;
        uint32_t duration = (ch.config.servoEnd - ch.config.servoStart);
        duration = duration * 10 * ch.control.value / 255 + ch.config.servoStart * 10;
        TCB1.CCMP = duration & 0xffff;
        TCB1.CNT = 0;
        gpio::high(activeServoPin_);
        TCB1.CTRLA |= TCB_ENABLE_bm; 
    }

    static void terminateControlPulse() __attribute__((always_inline)) {
        switch (activeServoPin_) {
            case L1_PIN: // PB5
                PORTB.OUTCLR = (1 << 5);
                break;
            case L2_PIN: // PB4
                PORTB.OUTCLR = (1 << 5);
                break;
            case R1_PIN: // PC3
                PORTC.OUTCLR = (1 << 3);
                break;
            case R2_PIN: // PC2
                PORTC.OUTCLR = (1 << 2);
                break;
        }
    }
    //}@

    /** \name Analog inputs
     
        We use both ADC0 and ADC1 to measure the analog values of the custom channels. The ADCs cycle through the connected channels. 

        Uses the ADC to read the analog inputs, if any. 
     */
    //@{

    static void initializeAnalogInputs() {

        // voltage reference to 1.1V (internal for the temperature sensor)
        VREF.CTRLA &= ~ VREF_ADC0REFSEL_gm;
        VREF.CTRLA |= VREF_ADC0REFSEL_1V1_gc;
        // TODO the above for ADC1 too



        ADC0.CTRLA = ADC_RESSEL_8BIT_gc;
        ADC0.CTRLB = ADC_SAMPNUM_ACC64_gc; 
        ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
        ADC0.CTRLD = 0; // no sample delay, no init delay
        ADC0.SAMPCTRL = 0;
        ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;

        ADC1.CTRLA = ADC_RESSEL_8BIT_gc;
        ADC1.CTRLB = ADC_SAMPNUM_ACC64_gc; 
        ADC1.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
        ADC1.CTRLD = 0; // no sample delay, no init delay
        ADC1.SAMPCTRL = 0;
        ADC1.MUXPOS = ADC_MUXPOS_INTREF_gc;
    
        ADC0.CTRLA |= ADC_ENABLE_bm;
        ADC1.CTRLA |= ADC_ENABLE_bm;
    }

    static void checkAnalogIn() {
        if (ADC0.INTFLAGS & ADC_RESRDY_bm) {
            uint8_t muxpos = ADC0.MUXPOS;
            uint8_t value = ADC0.RES / 64; // 64 samples
            switch (muxpos) {
                case ADC_MUXPOS_INTREF_gc:
                    // TODO deal with the VCC we have just measured
                    break;
                case ADC_MUXPOS_AIN8_gc:
                    if (l1_.config.mode == channel::CustomIO::Mode::AnalogIn)
                        feedback_.l1.value = value;
                    break;
                case ADC_MUXPOS_AIN9_gc:
                    if (l2_.config.mode == channel::CustomIO::Mode::AnalogIn)
                        feedback_.l2.value = value;
                    break;
            }
            switch (muxpos) {
                case ADC_MUXPOS_INTREF_gc:
                    if (l1_.config.mode == channel::CustomIO::Mode::AnalogIn) {
                        ADC0.MUXPOS = ADC_MUXPOS_AIN8_gc;
                        break;
                    }
                    // fallthrough
                case ADC_MUXPOS_AIN8_gc:
                    if (l2_.config.mode == channel::CustomIO::Mode::AnalogIn) {
                        ADC0.MUXPOS = ADC_MUXPOS_AIN9_gc;
                        break;
                    }
                    // fallthrough
                case ADC_MUXPOS_AIN9_gc:
                default:
                    ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
                    break;
            }
            ADC0.CTRLA |= ADC_ENABLE_bm;
        }
        if (ADC1.INTFLAGS & ADC_RESRDY_bm) {
            uint8_t muxpos = ADC1.MUXPOS;
            uint8_t value = ADC1.RES / 64; // 64 samples
            switch (muxpos) {
                case ADC_MUXPOS_INTREF_gc:
                    // TODO deal with the VCC we have just measured
                    break;
                case ADC_MUXPOS_AIN9_gc:
                    if (r1_.config.mode == channel::CustomIO::Mode::AnalogIn)
                        feedback_.r1.value = value;
                    break;
                case ADC_MUXPOS_AIN8_gc:
                    if (r2_.config.mode == channel::CustomIO::Mode::AnalogIn)
                        feedback_.r2.value = value;
                    break;
            }
            switch (muxpos) {
                case ADC_MUXPOS_INTREF_gc:
                    if (r1_.config.mode == channel::CustomIO::Mode::AnalogIn) {
                        ADC1.MUXPOS = ADC_MUXPOS_AIN9_gc;
                        break;
                    }
                    // fallthrough
                case ADC_MUXPOS_AIN9_gc:
                    if (r2_.config.mode == channel::CustomIO::Mode::AnalogIn) {
                        ADC1.MUXPOS = ADC_MUXPOS_AIN8_gc;
                        break;
                    }
                    // fallthrough
                case ADC_MUXPOS_AIN8_gc:
                default:
                    ADC1.MUXPOS = ADC_MUXPOS_INTREF_gc;
                    break;
            }
            ADC1.CTRLA |= ADC_ENABLE_bm;
        }
    }
    //}@

    /** \name PWM Outputs 

        TCA in split mode is used to provide 4 8-bit PWM channels (out of 6 it supports). All low channels (0, 1 and 2) as well as high channel 0 are used:

        Low 0 :  XR2, cmp and ovf interrupts are used to start/stop the pulse on the pin 
        Low 1 :  XL2, TCA-WO1 in alternate location 
        Low 2 :  XL1, TCA-WO2 in alternate location
        High 0 : XR1, TCA-WO3 in alternate location
     */
    //@{

    static void initializePWMOutputs() {
        // enable alternate locations of TCA-WO1 and TCA-WO2
        PORTMUX.CTRLC = PORTMUX_TCA01_bm | PORTMUX_TCA02_bm | PORTMUX_TCA03_bm;

        // initialize TCA for PWM outputs on the configurable channels. We use split mode
        TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm; // enable split mode
        TCA0.SPLIT.CTRLB = 0; // disable all outputs    
        // this gives us ~600Hz PWM frequency @ 10MHz
        TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV64_gc | TCA_SPLIT_ENABLE_bm; 
    }

    static void setPWM(gpio::Pin pin, uint8_t value) {
        switch (pin) {
            case L1_PIN:
                gpio::output(L1_PIN);
                TCA0.SPLIT.LCMP2 = value;
                TCA0.SPLIT.CTRLB |= TCA_SPLIT_LCMP2EN_bm;
                break;
            case L2_PIN:
                gpio::output(L2_PIN);
                TCA0.SPLIT.LCMP1 = value;
                TCA0.SPLIT.CTRLB |= TCA_SPLIT_LCMP1EN_bm;
                break;
            case R1_PIN:
                gpio::output(R1_PIN);
                TCA0.SPLIT.HCMP0 = value;
                TCA0.SPLIT.CTRLB |= TCA_SPLIT_HCMP0EN_bm;
                break;
            case R2_PIN:
                gpio::output(R2_PIN);
                TCA0.SPLIT.LCMP0 = value;
                TCA0.SPLIT.INTCTRL = TCA_SPLIT_LCMP0_bm | TCA_SPLIT_LUNF_bm;
                break;
        }
    }

    static void disablePWM(gpio::Pin pin) {
        switch (pin) {
            case L1_PIN:
                TCA0.SPLIT.CTRLB &= ~TCA_SPLIT_LCMP2EN_bm;
                break;
            case L2_PIN:
                TCA0.SPLIT.CTRLB &= ~TCA_SPLIT_LCMP1EN_bm;
                break;
            case R1_PIN:
                TCA0.SPLIT.CTRLB &= ~TCA_SPLIT_HCMP0EN_bm;
                break;
            case R2_PIN:
                TCA0.SPLIT.INTCTRL = 0;
                TCA0.SPLIT.INTFLAGS = 0xff; // and clear the flags
                break;
        }
    }

    //@}

    /** \name Audio 
     
        Audio runs using the TCB0 at 5Mhz and can output to any of the custom IO channels. Enabling the tone enables the interrupt on the timer oveflow which toggles the appropriate pin. The timer is thus run at twice the frequency of the required tone, given the tone range of 40Hz - 20kHz.
     */
    //@{

    static inline uint16_t freq_ = 0;
    static inline uint8_t toneTickDuration_ = 0;
    static inline uint8_t toneEffect_ = 0;

    static void initializeAudio() {
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc; // | TCB_ENABLE_bm;
        TCB0.INTCTRL = TCB_CAPT_bm;
    }

    static void tone(uint16_t freq) {
        TCB0.CCMP = 2500000 / freq;
        TCB0.CTRLA |= TCB_ENABLE_bm;
        freq_ = freq;
    }

    static void noTone() {
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc;
        freq_ = 0;
        switch (tone_.config.outputChannel) {
            case LegoRemote::CHANNEL_L1: // PB5
                PORTB.OUTSET = (1 << 5);
                break;
            case LegoRemote::CHANNEL_L2: // PB4
                PORTB.OUTSET = (1 << 4);
                break;
            case LegoRemote::CHANNEL_R1: // PC3
                PORTC.OUTSET = (1 << 3);
                break;
            case LegoRemote::CHANNEL_R2: // PC2
                PORTC.OUTSET = (1 << 2);
                break;
        }
    }

    static void startToneEffect(channel::ToneEffect::Control const * ctrl) {
        tone_.control = *ctrl;
        toneTickDuration_ = tone_.control.transition / 10 * 64 / 100; 
        if (toneTickDuration_ == 0)
            toneTickDuration_ = 1;
        switch (tone_.control.effect) {
            case channel::ToneEffect::Effect::Tone:
                tone(tone_.control.freq);
                toneEffect_ = tone_.control.transition == 0 ? 0 : toneTickDuration_;
                break;
            case channel::ToneEffect::Effect::Wail:
                tone(tone_.control.freq);
                toneEffect_ = 1;
                break;
            case channel::ToneEffect::Effect::Yelp:
                tone(tone_.control.freq);
                break;
            case channel::ToneEffect::Effect::HiLow:
                tone(tone_.control.freq);
                toneEffect_ = toneTickDuration_;
                break;
            case channel::ToneEffect::Effect::Pulse:
                tone(tone_.control.freq);
                toneEffect_ = toneTickDuration_;
                break;
        }
    }

    static void toneEffectTick() {
        if (tone_.config.outputChannel == 0)
            return;
        switch (tone_.control.effect) {
            case channel::ToneEffect::Effect::Tone:
                if (toneEffect_ > 0 && --toneEffect_ == 0)
                    noTone();
                break;
            case channel::ToneEffect::Effect::Wail:
                if (toneEffect_) {
                    tone(freq_ + (tone_.control.freq2 - tone_.control.freq) / toneTickDuration_);
                    if (freq_ >= tone_.control.freq2)
                        toneEffect_ = 0;
                } else {
                    tone(freq_ - (tone_.control.freq2 - tone_.control.freq) / toneTickDuration_);
                    if (freq_ <= tone_.control.freq)
                        toneEffect_ = 1;
                }
                break;
            case channel::ToneEffect::Effect::Yelp:
                if (freq_ >= tone_.control.freq2)
                    tone(tone_.control.freq);
                else 
                    tone(freq_ + (tone_.control.freq2 - tone_.control.freq) / toneTickDuration_);
                break;
            case channel::ToneEffect::Effect::HiLow:
                if (--toneEffect_ == 0) {
                    tone(freq_ == tone_.control.freq ? tone_.control.freq2 : tone_.control.freq);
                    toneEffect_ = toneTickDuration_;
                }
                break;
            case channel::ToneEffect::Effect::Pulse:
                if (--toneEffect_ == 0) {
                    if (freq_ == 0) {
                        tone(tone_.control.freq);
                        toneEffect_ = tone_.control.freq2 / 10 * 64 / 100;
                    } else {
                        noTone();
                        toneEffect_ = toneTickDuration_;                    
                    }
                }
                break;
        }
    }

    static void toneFlip() __attribute__((always_inline)) {
                PORTB.OUTTGL = (1 << 5);
                return;
        switch (tone_.config.outputChannel) {
            case LegoRemote::CHANNEL_L1: // PB5
                PORTB.OUTTGL = (1 << 5);
                break;
            case LegoRemote::CHANNEL_L2: // PB4
                PORTB.OUTTGL = (1 << 4);
                break;
            case LegoRemote::CHANNEL_R1: // PC3
                PORTC.OUTTGL = (1 << 3);
                break;
            case LegoRemote::CHANNEL_R2: // PC2
                PORTC.OUTTGL = (1 << 2);
                break;
        }
    }
    //@}

    /** \name RGB Strip Control & Effects 
     
        The first LED is the comms led on the remote brick and is controlled independently. The rest of the LEDs are user specific. 
     */
    //@{

    static inline NeopixelStrip<9> rgbColors_{NEOPIXEL_PIN};
    static inline ColorStrip<9> colors_;


    static void rgbEffectTick() {
        if (! rgbColors_[0].moveTowards(colors_[0])) {
            if (rgbColors_[0] == Color::Black()) {
                if (!paired())
                    colors_[0] = Color::Blue();
                else if (feedback_.device.connectionLost())
                    colors_[0] = Color::Red();
                else 
                    colors_[0] = Color::Green();
            } else {
                colors_[0] = Color::Black();
            }
        }
        rgbColors_.update();
    }
    //@}

}; // Remote

/** The TCB only fires when the currently multiplexed output is a servo motor. When it fires, we first pull the output low to terminate the control pulse and then disable the timer.
 */
ISR(TCB1_INT_vect) {
    TCB1.INTFLAGS = TCB_CAPT_bm;
    TCB1.CTRLA &= ~TCB_ENABLE_bm;
    Remote::terminateControlPulse();
}

ISR(TCB0_INT_vect) {
    TCB0.INTFLAGS = TCB_CAPT_bm;
    Remote::toneFlip();
}

ISR(TCA0_LUNF_vect) {
    TCA0.SPLIT.INTFLAGS = TCA_SPLIT_LUNF_bm;
    static_assert(Remote::R2_PIN == 12); // PC2
    PORTC.OUTCLR = (1 << 2);
}

ISR(TCA0_LCMP0_vect) {
    TCA0.SPLIT.INTFLAGS = TCA_SPLIT_LCMP0_bm;
    static_assert(Remote::R2_PIN == 12); // PC2
    PORTC.OUTSET = (1 << 2);
}

void setup() {
    Remote::initialize();
}

void loop() {
    Remote::loop();
}