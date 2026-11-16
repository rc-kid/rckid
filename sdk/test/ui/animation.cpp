#include <platform/tests.h>
#include <kiddo/ui/animation.h>

using namespace kiddo;
using namespace kiddo::ui;

TEST(easing, identity) {
    EXPECT(easing::identity(FixedRatio{0.0f}) == FixedRatio{0.0f});
    EXPECT(easing::identity(FixedRatio{0.5f}) == FixedRatio{0.5f});
    EXPECT(easing::identity(FixedRatio{1.0f}) == FixedRatio{1.0f});
}