/** \page backend_XXXYYY XXXYYY 

    This is RCKid backend template. The functions below have to be implemented in order to port the RCKid SDK to a new platform.
 */

#ifndef RCKID_BACKEND_XXXYYY
#error "You are building XXXYYY backend without the indicator macro"
#endif

namespace rckid {

    // forware declaration for the ionternal memory reset function
    void memoryReset();

    // forward declaration of the bsod function
    NORETURN(void bsod(uint32_t error, uint32_t arg, uint32_t line = 0, char const * file = nullptr));

    // forward declaration of memory stack protection check
    void StackProtection::check();

    namespace filesystem {
        void initialize();
    }

    void fatalError(uint32_t error, uint32_t arg,  uint32_t line, char const * file) {
        UNIMPLEMENTED;
        bsod(error, arg, line, file);
    }

    Writer debugWrite() {
        UNIMPLEMENTED;
    }

    uint8_t debugRead(bool echo) {
        UNIMPLEMENTED;
    }

    void initialize([[maybe_unused]] int argc, [[maybe_unused]] char * argv[]) {
        UNIMPLEMENTED;
    }

    void tick() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void yield() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void keepAlive() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    uint32_t uptimeUs() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    uint64_t uptimeUs64() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    TinyDateTime now() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    // io

    bool btnDown(Btn b) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    bool btnPressed(Btn b) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    bool btnReleased(Btn b) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void btnClear(Btn b) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    int16_t accelX() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    int16_t accelY() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    int16_t accelZ() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    int16_t gyroX() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    int16_t gyroY() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    int16_t gyroZ() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    uint16_t lightAmbient() {
        StackProtection::check();
        UNIMPLEMENTED;        
    }
    
    uint16_t lightUV() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    // display

    void displayOn() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void displayOff() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void displayClear(ColorRGB color) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    DisplayRefreshDirection displayRefreshDirection() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void displaySetRefreshDirection(DisplayRefreshDirection value) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    uint8_t displayBrightness() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void displaySetBrightness(uint8_t value) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    Rect displayUpdateRegion() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void displaySetUpdateRegion(Rect value) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void displaySetUpdateRegion(Coord width, Coord height) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    bool displayUpdateActive() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void displayWaitUpdateDone() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void displayWaitVSync() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void displayUpdate(uint16_t const * pixels, uint32_t numPixels, DisplayUpdateCallback callback) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void displayUpdate(uint16_t const * pixels, uint32_t numPixels) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    // audio

    bool audioHeadphones() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    bool audioPaused() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    bool audioPlayback() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    bool audioRecording() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    uint8_t audioVolume() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void audioSetVolume(uint8_t value) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void audioPlay(DoubleBuffer<int16_t> & buffer, uint32_t sampleRate, AudioCallback cb) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void audioPause() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void audioResume() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void audioStop() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    // SD Card access

    uint32_t sdCapacity() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    bool sdReadBlocks(uint32_t start, uint8_t * buffer, uint32_t numBlocks) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    bool sdWriteBlocks(uint32_t start, uint8_t const * buffer, uint32_t numBlocks) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    // Cartridge filesystem access

    uint32_t cartridgeCapacity() { 
        StackProtection::check();
        UNIMPLEMENTED;
    }

    uint32_t cartridgeWriteSize() { 
        StackProtection::check();
        UNIMPLEMENTED;
        return 256; 
    }

    uint32_t cartridgeEraseSize() { 
        StackProtection::check();
        UNIMPLEMENTED;
        return 4096; 
    }

    void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void cartridgeWrite(uint32_t start, uint8_t const * buffer) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void cartridgeErase(uint32_t start) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    // rumbler

    void rumblerEffect(RumblerEffect const & effect) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    // rgb

    void rgbEffect(uint8_t rgb, RGBEffect const & effect) {
        StackProtection::check();
        UNIMPLEMENTED;
    }
    
    void rgbEffects(RGBEffect const & a, RGBEffect const & b, RGBEffect const & dpad, RGBEffect const & sel, RGBEffect const & start) {
        StackProtection::check();
        UNIMPLEMENTED;
    }
    
    void rgbOff() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    // memory

    bool memoryIsImmutable(void const * ptr) {
        StackProtection::check();
        // change this to specific platform implementation if immutable memory is supported
        return false;
    }

    // budget

    uint32_t budget() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    uint32_t budgetDaily() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void budgetSet(uint32_t seconds) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void budgetDailySet(uint32_t seconds) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void budgetReset() {
        StackProtection::check();
        UNIMPLEMENTED;
    }
}

extern "C" {

    void memset32(uint32_t * buffer, uint32_t size, uint32_t value) {
        while (size-- != 0)
            *(buffer++) = value;
    }

    void memset16(uint16_t * buffer, uint32_t size, uint16_t value) {
        while (size-- != 0)
            *(buffer++) = value;
    }
}