#include <platform/tests.h>
#include <rckid/audio/audio.h>

using namespace rckid;

TEST(audio, freqToPeriodUs) {
    FixedInt p = frequencyToPeriodUs(440); // 2272.7272 ms
    EXPECT(p.raw() == 0x8e0b); // dv 16 == 2272.7272
}