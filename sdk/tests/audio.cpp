#include <platform/tests.h>
#include <rckid/audio/audio.h>

using namespace rckid;

TEST(audio, freqToPeriodUs) {
    FixedInt p = audio::frequencyToPeriodUs(440); // 2272.7272 ms
    EXPECT(p.raw() == 0x8e0b); // dv 16 == 2272.7272
}

TEST(audio, convertToStereoInPlace) {
    int16_t buffer[8] = {100, -200, 300, -400, 0x7fff, 0x7fff, 0x7fff, 0x7fff};
    audio::convertToStereo(buffer, 4);
    EXPECT(buffer[0] == 100);
    EXPECT(buffer[1] == 100);
    EXPECT(buffer[2] == -200);
    EXPECT(buffer[3] == -200);
    EXPECT(buffer[4] == 300);
    EXPECT(buffer[5] == 300);
    EXPECT(buffer[6] == -400);
    EXPECT(buffer[7] == -400);
}