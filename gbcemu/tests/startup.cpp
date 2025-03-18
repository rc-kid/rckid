#include "gbctests.h"

namespace rckid::gbcemu {

    TEST(gbcemu, startup_flags) {
        GBCEmu gbc{};
        RUN(
            NOP,
        );
        EXPECT_FLAGS(Z);
    }

} // namespace rckid::gbcemu
