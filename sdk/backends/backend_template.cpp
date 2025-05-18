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
    NORETURN(void bsod(uint32_t error, uint32_t line = 0, char const * file = nullptr));

    // forward declaration of memory stack protection check
    void memoryCheckStackProtection();

    namespace filesystem {
        void initialize();
    }

    void fatalError(uint32_t error, uint32_t line, char const * file) {
        memoryReset();
        UNIMPLEMENTED;
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
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void yield() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    uint32_t uptimeUs() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    // io

    bool btnDown(Btn b) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    bool btnPressed(Btn b) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    bool btnReleased(Btn b) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void btnClear(Btn b) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    int16_t accelX() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    int16_t accelY() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    int16_t accelZ() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    int16_t gyroX() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    int16_t gyroY() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    int16_t gyroZ() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    uint16_t lightAmbient() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;        
    }
    
    uint16_t lightUV() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    // display

    void displayOn() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void displayOff() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    DisplayRefreshDirection displayRefreshDirection() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void displaySetRefreshDirection(DisplayRefreshDirection value) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    uint8_t displayBrightness() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void displaySetBrightness(uint8_t value) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    Rect displayUpdateRegion() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void displaySetUpdateRegion(Rect value) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void displaySetUpdateRegion(Coord width, Coord height) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    bool displayUpdateActive() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void displayWaitUpdateDone() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void displayWaitVSync() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void displayUpdate(uint16_t const * pixels, uint32_t numPixels, DisplayUpdateCallback callback) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void displayUpdate(uint16_t const * pixels, uint32_t numPixels) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    // audio

    void audioStreamRefill(void * buffer, unsigned int samples) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    bool audioHeadphones() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    bool audioPaused() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    bool audioPlayback() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    bool audioRecording() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    uint8_t audioVolume() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void audioSetVolume(uint8_t value) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void audioPlay(DoubleBuffer<int16_t> & buffer, uint32_t sampleRate, AudioCallback cb) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void audioPause() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void audioResume() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void audioStop() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    // SD Card access

    uint32_t sdCapacity() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    bool sdReadBlocks(uint32_t start, uint8_t * buffer, uint32_t numBlocks) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    bool sdWriteBlocks(uint32_t start, uint8_t const * buffer, uint32_t numBlocks) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    // Cartridge filesystem access

    uint32_t cartridgeCapacity() { 
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    uint32_t cartridgeWriteSize() { 
        memoryCheckStackProtection();
        UNIMPLEMENTED;
        return 256; 
    }

    uint32_t cartridgeEraseSize() { 
        memoryCheckStackProtection();
        UNIMPLEMENTED;
        return 4096; 
    }

    void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void cartridgeWrite(uint32_t start, uint8_t const * buffer) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    void cartridgeErase(uint32_t start) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    // rumbler

    void rumblerEffect(RumblerEffect const & effect) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    // rgb

    void rgbEffect(uint8_t rgb, RGBEffect const & effect) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }
    
    void rgbEffects(RGBEffect const & a, RGBEffect const & b, RGBEffect const & dpad, RGBEffect const & sel, RGBEffect const & start) {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }
    
    void rgbOff() {
        memoryCheckStackProtection();
        UNIMPLEMENTED;
    }

    // memory

    bool memoryIsImmutable(void const * ptr) {
        memoryCheckStackProtection();
        // change this to specific platform implementation if immutable memory is supported
        return false;
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