/** \page backend_mk3 Mk III 

    Mark III, currently in development whose hardware specifications are still in progress.
*/

#ifndef RCKID_BACKEND_MK3
#error "You are building fantasy (RayLib) backend without the indicator macro"
#endif

#include "rckid/rckid.h"

namespace rckid {

    // forward declaration of the bsod function
    NORETURN(void bsod(uint32_t error, uint32_t line = 0, char const * file = nullptr));

    namespace filesystem {
        void initialize();
    }

    void fatalError(uint32_t error, uint32_t line, char const * file) {
        UNIMPLEMENTED;
    }

    Writer debugWrite() {
        UNIMPLEMENTED;
    }

    void initialize([[maybe_unused]] int argc, [[maybe_unused]] char const * argv[]) {
        UNIMPLEMENTED;
    }

    void tick() {
        UNIMPLEMENTED;
    }

    void yield() {
        UNIMPLEMENTED;
    }

    uint32_t uptimeUs() {
        UNIMPLEMENTED;
    }

    // io

    bool btnDown(Btn b) {
        UNIMPLEMENTED;
    }

    bool btnPressed(Btn b) {
        UNIMPLEMENTED;
    }

    bool btnReleased(Btn b) {
        UNIMPLEMENTED;
    }

    int16_t accelX() {
        UNIMPLEMENTED;
    }

    int16_t accelY() {
        UNIMPLEMENTED;
    }

    int16_t accelZ() {
        UNIMPLEMENTED;
    }

    int16_t gyroX() {
        UNIMPLEMENTED;
    }

    int16_t gyroY() {
        UNIMPLEMENTED;
    }

    int16_t gyroZ() {
        UNIMPLEMENTED;
    }

    uint16_t lightAmbient() {
        UNIMPLEMENTED;        
    }
    
    uint16_t lightUV() {
        UNIMPLEMENTED;
    }

    // display

    DisplayRefreshDirection displayRefreshDirection() {
        UNIMPLEMENTED;
    }

    void displaySetRefreshDirection(DisplayRefreshDirection value) {
        UNIMPLEMENTED;
    }

    uint8_t displayBrightness() {
        UNIMPLEMENTED;
    }

    void displaySetBrightness(uint8_t value) {
        UNIMPLEMENTED;
    }

    Rect displayUpdateRegion() {
        UNIMPLEMENTED;
    }

    void displaySetUpdateRegion(Rect value) {
        UNIMPLEMENTED;
    }

    bool displayUpdateActive() {
        UNIMPLEMENTED;
    }

    void displayWaitUpdateDone() {
        UNIMPLEMENTED;
    }

    void displayWaitVSync() {
        UNIMPLEMENTED;
    }

    void displayUpdate(uint16_t const * pixels, uint32_t numPixels, DisplayUpdateCallback callback) {
        UNIMPLEMENTED;
    }

    void displayUpdate(uint16_t const * pixels, uint32_t numPixels) {
        UNIMPLEMENTED;
    }

    // audio

    void audioStreamRefill(void * buffer, unsigned int samples) {
        UNIMPLEMENTED;
    }

    bool audioHeadphones() {
        UNIMPLEMENTED;
    }

    bool audioPaused() {
        UNIMPLEMENTED;
    }

    bool audioPlayback() {
        UNIMPLEMENTED;
    }

    bool audioRecording() {
        UNIMPLEMENTED;
    }

    uint8_t audioVolume() {
        UNIMPLEMENTED;
    }

    void audioSetVolume(uint8_t value) {
        UNIMPLEMENTED;
    }

    void audioPlay(DoubleBuffer<int16_t> & buffer, uint32_t sampleRate, AudioCallback cb) {
        UNIMPLEMENTED;
    }

    void audioPause() {
        UNIMPLEMENTED;
    }

    void audioResume() {
        UNIMPLEMENTED;
    }

    void audioStop() {
        UNIMPLEMENTED;
    }

    // SD Card access

    uint32_t sdCapacity() {
        UNIMPLEMENTED;
    }

    bool sdReadBlocks(uint32_t start, uint8_t * buffer, uint32_t numBlocks) {
        UNIMPLEMENTED;
    }

    bool sdWriteBlocks(uint32_t start, uint8_t const * buffer, uint32_t numBlocks) {
        UNIMPLEMENTED;
    }

    // Cartridge filesystem access

    uint32_t cartridgeCapacity() { 
        UNIMPLEMENTED;
    }

    uint32_t cartridgeWriteSize() { 
        UNIMPLEMENTED;
        return 256; 
    }

    uint32_t cartridgeEraseSize() { 
        UNIMPLEMENTED;
        return 4096; 
    }

    void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes) {
        UNIMPLEMENTED;
    }

    void cartridgeWrite(uint32_t start, uint8_t const * buffer) {
        UNIMPLEMENTED;
    }

    void cartridgeErase(uint32_t start) {
        UNIMPLEMENTED;
    }

}