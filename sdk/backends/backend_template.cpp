/** \page backend_XXXYYY XXXYYY 

    This is RCKid backend template. The functions below have to be implemented in order to port the RCKid SDK to a new platform.
 */
#ifndef RCKID_BACKEND_XXXYYY
#error "You are building XXXYYY backend without the indicator macro"
#endif

#include "rckid/rckid.h"



namespace rckid {

    // forward declaration of the bsod function
    NORETURN(void bsod(uint32_t error, uint32_t line = 0, char const * file = nullptr));

    void fatalError(uint32_t error, uint32_t line, char const * file) {
    }

    Writer debugWrite() {
    }

    void initialize([[maybe_unused]] int argc, [[maybe_unused]] char const * argv[]) {
    }

    void tick() {
    }

    void yield() {
    }

    uint32_t uptimeUs() {
    }

    // io

    bool btnDown(Btn b) {
    }

    bool btnPressed(Btn b) {
    }

    bool btnReleased(Btn b) {
    }

    int16_t accelX() {
    }

    int16_t accelY() {
    }

    int16_t accelZ() {
    }

    int16_t gyroX() {
    }

    int16_t gyroY() {
    }

    int16_t gyroZ() {
    }

    // display

    DisplayResolution displayResolution() {
    }

    void displaySetResolution(DisplayResolution value) {
    }

    DisplayRefreshDirection displayRefreshDirection() {
    }

    void displaySetRefreshDirection(DisplayRefreshDirection value) {
    }

    uint8_t displayBrightness() {
    }

    void displaySetBrightness(uint8_t value) {
    }

    Rect displayUpdateRegion() {
    }

    void displaySetUpdateRegion(Rect value) {
    }

    bool displayUpdateActive() {
    }

    void displayWaitUpdateDone() {
    }

    void displayWaitVSync() {
    }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels, DisplayUpdateCallback callback) {
    }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels) {
    }

    // audio

    void audioStreamRefill(void * buffer, unsigned int samples) {
    }

    bool audioHeadphones() {
    }

    bool audioPaused() {
    }

    bool audioPlayback() {
    }

    bool audioRecording() {
    }

    uint8_t audioVolume() {
    }

    void audioSetVolume(uint8_t value) {
    }

    void audioPlay(DoubleBuffer<int16_t> & buffer, uint32_t sampleRate, AudioCallback cb) {
    }

    void audioPause() {
    }

    void audioResume() {
    }

    void audioStop() {
    }

    // SD Card access

    uint32_t sdCapacity() {
    }

    bool sdReadBlocks(uint32_t start, uint8_t * buffer, uint32_t numBlocks) {
    }

    bool sdWriteBlocks(uint32_t start, uint8_t const * buffer, uint32_t numBlocks) {
    }

    // Cartridge filesystem access

    uint32_t cartridgeCapacity() { 
    }

    uint32_t cartridgeWriteSize() { return 256; }

    uint32_t cartridgeEraseSize() { return 4096; }

    void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes) {
    }

    void cartridgeWrite(uint32_t start, uint8_t const * buffer) {
    }

    void cartridgeErase(uint32_t start) {
    }

}