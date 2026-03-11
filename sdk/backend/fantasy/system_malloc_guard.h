#pragma once

#include <rckid/error.h>

namespace rckid::internal::memory {

    extern uint32_t useSystemMalloc;

    class SystemMallocGuard {
    public:
        SystemMallocGuard():
            old_{useSystemMalloc} {
            acquire();
        }

        ~SystemMallocGuard() {
            release();
        }

        void release() {
            ASSERT(useSystemMalloc > 0);
            --useSystemMalloc;
            ASSERT(old_ == useSystemMalloc);
        }

        void acquire() {
            ASSERT(old_ == useSystemMalloc);
            ++useSystemMalloc;
        }
    private:
        uint32_t old_;

    }; // SystemMallocGuard


}