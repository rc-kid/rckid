#pragma once

#include <rckid/ui/app.h>

extern "C" {
    //bool tud_msc_start_stop_cb(uint8_t, uint8_t, bool, bool);
    int32_t tud_msc_read10_cb(uint8_t, uint32_t, uint32_t, void *, uint32_t);
    int32_t tud_msc_write10_cb(uint8_t, uint32_t, uint32_t, uint8_t*, uint32_t);
}

namespace rckid {


    class DataSync : public ui::App<void> {
    public:

        virtual String name() const override { return "Data Sync"; }

        static bool active() {
            return false;
            //UNIMPLEMENTED;
        }

        static void disconnect() {
            //UNIMPLEMENTED;
        }

        static void connect() {
            //UNIMPLEMENTED;
        }

    private:
        friend int32_t ::tud_msc_read10_cb(uint8_t, uint32_t, uint32_t, void *, uint32_t);
        friend int32_t ::tud_msc_write10_cb(uint8_t, uint32_t, uint32_t, uint8_t*, uint32_t);
        


        // true if cable is attached, false otherwise
        bool attached_ = false;
        // true if currently connected as MSC
        bool connected_ = false;
        uint32_t blocksRead_ = 0;
        uint32_t blocksWrite_ = 0;

        static inline DataSync * instance_ = nullptr;
    };
} // namespace rckid