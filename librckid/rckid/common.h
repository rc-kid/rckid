/** RCKId's status. 

    Contains the current state of all buttons as well as the audio and power events, all monitored by the AVR. 
 */
class Status {
public:
    bool dpadLeft() const { return raw_ & DPAD_LEFT; }
    bool dpadRight() const { return raw_ & DPAD_RIGHT; }
    bool dpadUp() const { return raw_ & DPAD_UP; }
    bool dpadDown() const { return raw_ & DPAD_DOWN; }
    bool btnA() const { return raw_ & BTN_A; }
    bool btnB() const { return raw_ & BTN_B; }
    bool btnSelect() const { return raw_ & BTN_SEL; }
    bool btnStart() const { return raw_ & BTN_START; }
    bool btnHome() const { return raw_ & BTN_HOME; }
    bool btnVolUp() const { return raw_ & BTN_VOL_UP; }
    bool btnVolDown() const { return raw_ & BTN_VOL_DOWN; }
    bool charging() const { return raw_ & CHARGING; }
    bool dcPower() const { return raw_ & DC_POWER; }
    bool headphones() const { return raw_ & HEADPHONES; }
    bool audioEnabled() const { return raw_ & AUDIO_ENABLED; }

    void setDpadLeft(bool value = true) { value ? raw_ |= DPAD_LEFT : raw_ &= ~DPAD_LEFT; }
    void setDpadRight(bool value = true) { value ? raw_ |= DPAD_RIGHT : raw_ &= ~DPAD_RIGHT; }
    void setDpadUp(bool value = true) { value ? raw_ |= DPAD_UP : raw_ &= ~DPAD_UP; }
    void setDpadDown(bool value = true) { value ? raw_ |= DPAD_DOWN : raw_ &= ~DPAD_DOWN; }
    void setBtnA(bool value = true) { value ? raw_ |= BTN_A : raw_ &= ~BTN_A; }
    void setBtnB(bool value = true) { value ? raw_ |= BTN_B : raw_ &= ~BTN_B; }
    void setBtnSelect(bool value = true) { value ? raw_ |= BTN_SEL : raw_ &= ~BTN_SEL; }
    void setBtnStart(bool value = true) { value ? raw_ |= BTN_START : raw_ &= ~BTN_START; }
    void setBtnHome(bool value = true) { value ? raw_ |= BTN_HOME : raw_ &= ~BTN_HOME; }
    void setBtnVolUp(bool value = true) { value ? raw_ |= BTN_VOL_UP : raw_ &= ~BTN_VOL_UP; }
    void setBtnVolDown(bool value = true) { value ? raw_ |= BTN_VOL_DOWN : raw_ &= ~BTN_VOL_DOWN; }
    void setCharging(bool value = true) { value ? raw_ |= CHARGING : raw_ &= ~CHARGING; }
    void setDCPower(bool value = true) { value ? raw_ |= DC_POWER : raw_ &= ~DC_POWER; }
    void setHeadphones(bool value = true) { value ? raw_ |= HEADPHONES : raw_ &= ~HEADPHONES; }
    void setAudioEnabled(bool value = true) { value ? raw_ |= AUDIO_ENABLED : raw_ &= ~AUDIO_ENABLED; }

private:
    static constexpr uint16_t DPAD_LEFT = 1 << 0;
    static constexpr uint16_t DPAD_RIGHT = 1 << 1;
    static constexpr uint16_t DPAD_UP = 1 << 2;
    static constexpr uint16_t DPAD_DOWN = 1 << 3;
    static constexpr uint16_t BTN_A = 1 << 4;
    static constexpr uint16_t BTN_B = 1 << 5;
    static constexpr uint16_t BTN_SEL = 1 << 6;
    static constexpr uint16_t BTN_START = 1 << 7;
    static constexpr uint16_t BTN_HOME = 1 << 8;
    static constexpr uint16_t BTN_VOL_UP = 1 << 9;
    static constexpr uint16_t BTN_VOL_DOWN = 1 << 10;
    static constexpr uint16_t CHARGING = 1 << 11;
    static constexpr uint16_t DC_POWER = 1 << 12;
    static constexpr uint16_t HEADPHONES = 1 << 13;
    static constexpr uint16_t AUDIO_ENABLED = 1 << 14;
    // 15
    uint16_t raw_ = 0;
} __attribute__((packed)); // Status

/** Extra information gathered by the AVR. 
 */
class Info {
public:

    void setVcc(uint16_t value) {

    }

    uint8_t backlight() const { return backlight_; }
    void setBacklight(uint8_t value) { backlight_ = value; }
private:
    uint8_t vcc_;
    uint8_t backlight_;
} __attribute__((packed)); // Info

/** The entire state of the device. 
    
    This is what the AVR chip will always return when asked to write data. Contains the status first, followed by all other data the AVR is in charge of or simply persisting. 
 */
class State {
public:

    Status status;
    Info info;

} __attribute__((packed)); // State