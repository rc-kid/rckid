#include <platform/tests.h>
#include <rckid/ui/animation.h>

using namespace rckid;
using namespace rckid::ui;

TEST(easing, identity) {
    EXPECT(easing::identity(FixedRatio{0.0f}) == FixedRatio{0.0f});
    EXPECT(easing::identity(FixedRatio{0.5f}) == FixedRatio{0.5f});
    EXPECT(easing::identity(FixedRatio{1.0f}) == FixedRatio{1.0f});
}