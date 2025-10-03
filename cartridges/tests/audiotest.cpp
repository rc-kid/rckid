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
    DoubleBuffer<int16_t> buf_{2048};
    Tone t;
    t.setWaveform(Waveform::Sine());
    //t.setWaveform(Waveform{assets::WaveformTriangle});
    t.setSampleRate(44100);
    t.on(440);
    audioPlay(buf_, 44100, [&](int16_t * buf, uint32_t size) {
        ++numTicks;
        return t.generateInto(buf, size);
    });
    while (true) {
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
        yield();
        cpu::delayMs(100);
        LOG(LL_INFO, "ticks: " << numTicks);
    }
}
