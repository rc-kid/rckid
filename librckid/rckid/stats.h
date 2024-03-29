#pragma once

#include <stdint.h>

namespace rckid {

    class App;
    class ST7789;

    /** Statistics of the RP2040 runtime. 
     
        Namespace class that provides various metrics about the device's performance. Automatically populated by the apps. 
     */
    class stats {
    public:

        static unsigned fps() { return fps_; }
        static unsigned systemUs() { return systemUs_; }
        static unsigned updateUs() { return updateUs_; }
        static unsigned drawUs() { return drawUs_; }
        static unsigned frameUs() { return frameUs_; }
        static unsigned idleUs() { return frameUs_ - systemUs_ - updateUs_ - drawUs_; }
        static unsigned idlePct() { return idleUs() * 100 / frameUs_; }

        static unsigned lastUpdateUs() { return updateUs_; }
        static unsigned lastUpdateWaitUs() { return updateWaitUs_; }
        static unsigned lastVSyncWaitUs() { return vsyncWaitUs_; }

    private:

        friend class ST7789;
        friend class App;

        static inline unsigned fps_;
        static inline unsigned fpsCounter_;
        static inline unsigned systemUs_;
        static inline unsigned updateUs_;
        static inline unsigned drawUs_;
        static inline unsigned frameUs_;

        static inline uint32_t nextFpsTick_;

        static inline unsigned displayUpdateUs_ = 0;
        static inline unsigned updateWaitUs_ = 0;
        static inline unsigned vsyncWaitUs_ = 0;
        static inline uint32_t updateStart_ = 0;

    }; // rckid::Stats
} // namespace rckid