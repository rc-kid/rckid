#include <rckid/rckid.h>

#include <rckid/audio/tone.h>

using namespace rckid;

void listI2CDevices() {
    debugWrite() << "I2C probe:\n";
    uint32_t n = 0;
    for (uint8_t i = 0; i < 127; ++i) {
        if (i2c::isPresent(i)) {
            debugWrite() << "    " << hex(i) << '\n';
            ++n;
        }
    }
    debugWrite() << "Found " << n << " devices\n";
}


/** Tests various hardware features. 
 */
int main() {
    initialize();
    /*
    DoubleBuffer<int16_t> buf_{2048};
    Tone t;
    t.setWaveform(Waveform::Sine());
    t.setSampleRate(44100);
    t.on(440);
    audioPlay(buf_, 44100, [&](int16_t * buf, uint32_t size) {
        return t.generateInto(buf, size);
    });
    */

    //TLV320 ac;
    //ac.standby();
    uint32_t i = 0;
    while (true) {
        /*
            LOG(LL_INFO, "headset: " << (uint8_t) ac.connectedHeadset());
            LOG(LL_INFO, "    btn: " << (uint8_t) ac.headsetButtonDown());
            LOG(LL_INFO, "  00-2e: " << bin(ac.r(0x00,0x2e)));
            LOG(LL_INFO, "  00-43: " << bin(ac.r(0x00,0x43)));
            LOG(LL_INFO, "  01-02: " << bin(ac.r(0x01,0x02)));
        */
        if (i % 2 == 0) {
            gpio::outputHigh(15);
        } else {
            gpio::outputLow(15);
        }
        LOG(LL_ERROR, "Tick " << (i++));
        //listI2CDevices();        
        uint32_t next = uptimeUs() + 1000000;
        while (uptimeUs() < next) {
            yield();
        }
    }

    /*
    listI2CDevices();
    while (true) {
        tick();
    }
        */
}
