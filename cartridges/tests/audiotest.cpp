#include <rckid/rckid.h>

#include <rckid/audio/tone.h>

using namespace rckid;

uint32_t numTicks = 0;

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


/** Basic audio test for the mkIII device. 
 */
int main() {
    initialize();
    DoubleBuffer<int16_t> buf_{2000};
    Tone t;
    t.setWaveform(Waveform::Sine());
    //t.setWaveform(Waveform{assets::WaveformTriangle});
    t.setSampleRate(44000);
    t.on(440);
    audioPlay(44000, [&](int16_t * & buf, uint32_t & size) {
        if (buf == nullptr) {
            buf = buf_.front();
            LOG(LL_INFO, "Buffer size is " << buf_.size());
            size = buf_.size() / 2;
            buf_.swap();
            t.generateInto(buf, size);
        }
        ++numTicks;
    });
    while (true) {
        yield(); /*
        cpu::delayMs(100);
        yield();
        cpu::delayMs(100);
        yield();
        cpu::delayMs(100);
        yield();
        cpu::delayMs(100);
        yield();
        cpu::delayMs(100);
        yield();
        cpu::delayMs(100);
        yield();
        cpu::delayMs(100);
        yield();
        cpu::delayMs(100);
        LOG(LL_INFO, "ticks: " << numTicks);
        */
    }
}
