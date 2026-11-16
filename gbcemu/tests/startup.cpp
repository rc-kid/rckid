#include "gbctests.h"

namespace kiddo::gbcemu {

    TEST(gbcemu, startup_flags) {
        GBCEmu gbc{"", nullptr};
        RUN(
            NOP,
        );
        EXPECT_FLAGS(Z);
    }

} // namespace kiddo::gbcemu
