#pragma once

#include <rckid/error.h>

namespace rckid::internal::memory {

    extern bool useSystemMalloc;

    class SystemMallocGuard {
    public:
        SystemMallocGuard(bool reentrant = false):
            old_{useSystemMalloc} {
            if (!reentrant || !old_)
                acquire();
        }

        ~SystemMallocGuard() {
            if (!old_)
                release();
        }

        void release() {
            ASSERT(useSystemMalloc == true);
            useSystemMalloc = false;
        }

        void acquire() {
            ASSERT(useSystemMalloc == false);
            useSystemMalloc = true;
        }
    private:
        bool old_;

    }; // SystemMallocGuard


}