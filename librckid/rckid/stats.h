#pragma once

#include <stdint.h>

namespace rckid {

    /** Statistics of the RP2040 runtime. 
     
        Namespace class that provides various metrics about the device's performance. Automatically populated by the apps. 
     */
    class stats {
    public:

        // timings for the app loop elements
        static unsigned waitTickUs() { return waitTickUs_; }
        static unsigned updateUs() { return updateUs_; }
        static unsigned tickUs() { return tickUs_; }
        static unsigned waitRenderUs() { return waitRenderUs_; }
        static unsigned drawUs() { return drawUs_; }
        static unsigned waitVSyncUs() { return waitVSyncUs_; }
        static unsigned renderUs() { return renderUs_; }

        static unsigned fps() { return fps_; }

        static unsigned lastUpdateUs() { return updateUs_; }
        static unsigned lastUpdateWaitUs() { return updateWaitUs_; }
        static unsigned lastVSyncWaitUs() { return vsyncWaitUs_; }

        static unsigned displayUpdateUs() { return displayUpdateUs_; }

    private:

        friend class ST7789;
        friend class BaseApp;
        friend void irqDMADone_();
        friend void yield();
        friend void tick();
        friend void * __wrap_malloc(size_t);
        friend void __wrap_free(void *);



        // time information for the app loop elements
        static inline uint32_t waitTickUs_ = 0;
        static inline uint32_t updateUs_ = 0;
        static inline uint32_t tickUs_ = 0;
        static inline uint32_t waitRenderUs_ = 0;
        static inline uint32_t drawUs_ = 0;
        static inline uint32_t waitVSyncUs_ = 0;
        static inline uint32_t renderUs_ = 0;
        
        // fps rendered by the current app
        static inline unsigned fps_;


        static inline unsigned displayUpdateUs_ = 0;
        static inline unsigned updateWaitUs_ = 0;
        static inline unsigned vsyncWaitUs_ = 0;
        static inline uint32_t updateStart_ = 0;

    }; // rckid::Stats
} // namespace rckid