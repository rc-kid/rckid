#include "rckid/app.h"
#include "rckid/stats.h"
#include "rckid/graphics/framebuffer.h"

using namespace rckid;

class SimpleApp : public App<FrameBuffer<Color256>> {
public:

protected:

    void update() override {

    }

    void draw() override {
        driver_.fill();
        driver_.textMultiline(0, 0) 
            << "SimpleApp:      " << t << "\n"
            << "FPS:            " << stats::fps() << "\n"
            << "waitTick:       " << stats::waitTickUs() << "\n"
            << "updateUs:       " << stats::updateUs() << "\n"
            << "tickUs:         " << stats::tickUs() << "\n"
            << "waitRender:     " << stats::waitRenderUs() << "\n"
            << "drawUs:         " << stats::drawUs() << "\n"
            << "waitVSync:      " << stats::waitVSyncUs() << "\n"
            << "render:         " << stats::renderUs() << "\n"
            << "display update: " << stats::displayUpdateUs() << "\n"
            << "tick update:    " << stats::tickUpdateUs() << "\n"
            << "I2C errors:     " << stats::i2cErrors();
        driver_.textMultiline(160, 0)
            << "AccelX:         " << accelX() << "\n"
            << "AccelY:         " << accelY() << "\n"
            << "AccelZ:         " << accelZ() << "\n"
            << "GyroX:          " << gyroX() << "\n"
            << "GyroY:          " << gyroY() << "\n"
            << "GyroZ:          " << gyroZ() << "\n"
            << "ALS:            " << lightAmbient() << "\n"
            << "UV:             " << lightUV() << "\n"
            << "VCC:            " << vcc() << "\n"
            << "VBatt:          " << vBatt() << "\n"
            << "Temp:           " << tempAvr() << "\n";
        ++t;
    }

    unsigned t = 0;



}; // SimpleApp

int main() {
    rckid::initialize();
    SimpleApp{}.run();
}