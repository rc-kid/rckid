#pragma once

#include "../app.h"
#include "../filesystem.h"
#include "../ui/form.h"
#include "../ui/label.h"
#include "../ui/image.h"

#if (defined PLATFORM_RP2350)
#include "tusb.h"
#endif

extern "C" {

    //bool tud_msc_start_stop_cb(uint8_t, uint8_t, bool, bool);
    int32_t tud_msc_read10_cb(uint8_t, uint32_t, uint32_t, void *, uint32_t);
    int32_t tud_msc_write10_cb(uint8_t, uint32_t, uint32_t, uint8_t*, uint32_t);
}

namespace rckid {

    /** SD Card as USB MSC

        Note that due to the USB speeds reachable with RP2350, the USB MSC performance is just barely useable. For any larger transfers, it is recommended to remove the SD card and use a proper card reader.

        DataSync is standalone mode app to ensure to attempts to read/write SD card from user code may happen while it is active.
     */
    class DataSync : public ui::Form<void>, App::StandaloneModeGuard {
    public:

        String name() const override { return "DataSync"; }

        DataSync():
            ui::Form<void>{}
        {
            icon_ = g_.addChild(new ui::Image{Icon{assets::icons_64::pen_drive}});
            info_ = g_.addChild(new ui::Label{Rect::XYWH(0, 150, 320, 20), "SD card not found"});
            status_ = g_.addChild(new ui::Label{Rect::XYWH(0, 180, 320, 20), "Disconnected"});
            icon_->setTransparent(true);
            icon_->setPos(160 - icon_->width() / 2, 60);
            status_->setHAlign(HAlign::Center);
            fs::mount(fs::Drive::SD);
            if (fs::isMounted()) {
                // load the bg image from SD card before we unmount it.
                ui::FormWidget::loadBackgroundImage();
                info_->setText(STR("SD card: " << sdCapacity() / 2 / 1024 << "MB, label " << fs::getLabel()));
                fs::unmount();
                connected_ = false;
                instance_ = this;
            }

        }

        ~DataSync() {
            instance_ = nullptr;
            // re-mount the SD card for normal use
            fs::mount(fs::Drive::SD);
        }

        void update() override {
            ui::Form<void>::update();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                btnClear(Btn::B);
                btnClear(Btn::Down);
                exit();
            }
#if (defined PLATFORM_RP2350)
            if (tud_ready() != attached_) {
                attached_ = ! attached_;
                LOG(LL_INFO, "USB cable " << (attached_ ? "attached" : "detached"));
                if (! attached_ && connected_)
                    doDisconnect();
                instance_ = this;
            }
#endif
        }

        void draw() override {
            if (connected_) {
                status_->setText(STR("Connected, r: " << blocksRead_ << ", w: " << blocksWrite_));
                // keep RCKid alive when connected over USB MSC
                keepAlive();
            } else if (attached_) {
                status_->setText(STR("Disconnected, r: " << blocksRead_ << ", w: " << blocksWrite_));
            } else {
                status_->setText(STR("Not Attached, r: " << blocksRead_ << ", w: " << blocksWrite_));
            }
            ui::Form<void>::draw();
        }

        static bool active() {
            if (instance_ == nullptr)
                return false;
            if (! instance_->connected_)
                instance_->connect();
            return true;
        }

        static void connect() {
            if (instance_ == nullptr)
                return;
            instance_->doConnect();
        }

        static void disconnect() {
            if (instance_ == nullptr)
                return;
            instance_->doDisconnect();
        }

    private:
        //friend bool ::tud_msc_start_stop_cb(uint8_t, uint8_t, bool, bool);
        friend int32_t ::tud_msc_read10_cb(uint8_t, uint32_t, uint32_t, void *, uint32_t);
        friend int32_t ::tud_msc_write10_cb(uint8_t, uint32_t, uint32_t, uint8_t*, uint32_t);

        ui::Image * icon_;
        ui::Label * info_;
        ui::Label * status_;

        void doConnect() {
            ASSERT(connected_ == false);
            connected_ = true;
            blocksRead_ = 0;
            blocksWrite_ = 0;
            LOG(LL_INFO, "MSC connected");
        }

        void doDisconnect() {
            if (connected_)
                connected_ = false;
            instance_ = nullptr;
            LOG(LL_INFO, "MSC disconnected");
        }

        static inline DataSync * instance_ = nullptr;

        // true if cable is attached, false otherwise
        bool attached_ = false;
        // true if currently connected as MSC
        bool connected_ = false;
        uint32_t blocksRead_ = 0;
        uint32_t blocksWrite_ = 0;

    }; // DataSync

} // namespace rckid