#pragma once

#include <stdint.h>

namespace rckid {

    /** Statistics of the RP2040 runtime. 
     
        Namespace class that provides various metrics about the device's performance. Automatically populated by the apps. 
     */
    class stats {
    public:

        // timings for the app loop elements
        static uint32_t waitTickUs() { return waitTickUs_; }
        static uint32_t updateUs() { return updateUs_; }
        static uint32_t tickUs() { return tickUs_; }
        static uint32_t waitRenderUs() { return waitRenderUs_; }
        static uint32_t drawUs() { return drawUs_; }
        static uint32_t waitVSyncUs() { return waitVSyncUs_; }
        static uint32_t renderUs() { return renderUs_; }

        static uint32_t fps() { return fps_; }




        static uint32_t i2cErrors() { return i2cErrors_; }

        static uint32_t lastUpdateUs() { return updateUs_; }

        static uint32_t displayUpdateUs() { return displayUpdateUs_; }
        static uint32_t tickUpdateUs() { return tickUpdateUs_; }

    private:

        friend class ST7789;
        friend class BaseApp;
        friend class DeviceWrapper;

        friend void irqDMADone_();
        friend void irqI2CDone_();
        friend void yield();
        friend void tick();
        friend void * __wrap_malloc(size_t);
        friend void __wrap_free(void *);


        static inline uint32_t ticks_ = 0;

        // time information for the app loop elements
        static inline uint32_t waitTickUs_ = 0;
        static inline uint32_t updateUs_ = 0;
        static inline uint32_t tickUs_ = 0;
        static inline uint32_t waitRenderUs_ = 0;
        static inline uint32_t drawUs_ = 0;
        static inline uint32_t waitVSyncUs_ = 0;
        static inline uint32_t renderUs_ = 0;
        
        // fps rendered by the current app
        static inline uint32_t fps_;

        static inline uint32_t i2cErrors_ = 0;


        static inline uint32_t displayUpdateUs_ = 0;
        static inline uint32_t tickUpdateUs_ = 0;

        static inline uint32_t displayUpdateStart_ = 0;
        static inline uint32_t tickUpdateStart_ = 0;

    }; // rckid::Stats
} // namespace rckid