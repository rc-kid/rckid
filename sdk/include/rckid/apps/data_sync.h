#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>

#include <assets/Iosevka24.h>
#include <assets/icons_64.h>

extern "C" {
    //bool tud_msc_start_stop_cb(uint8_t, uint8_t, bool, bool);
    int32_t tud_msc_read10_cb(uint8_t, uint32_t, uint32_t, void *, uint32_t);
    int32_t tud_msc_write10_cb(uint8_t, uint32_t, uint32_t, uint8_t*, uint32_t);
}

namespace rckid {


    class DataSync : public ui::App<void> {
    public:

        virtual String name() const override { return "Data Sync"; }

        DataSync() {
            using namespace ui;
            icon_ = addChild(new Image())
                << SetRect(Rect::XYWH(0, 60, 320, 64))
                << SetBitmap(assets::icons_64::pen_drive);
            info_ = addChild(new Label())
                << SetRect(Rect::XYWH(0, 140, 320, 24))
                << SetFont(assets::Iosevka24)
                << SetHAlign(HAlign::Center);
            status_ = addChild(new Label())
                << SetRect(Rect::XYWH(0, 160, 320, 24))
                << SetText("Disconnected")
                << SetFont(assets::Iosevka24)
                << SetHAlign(HAlign::Center);

            ASSERT(instance_ == nullptr);

            fs::mount(fs::Drive::SD);
            // TODO check also the SD card inserted pin
            if (fs::isMounted()) {
                info_->setText(STR("SD card: " << hal::fs::sdCapacityBlocks() / 2 / 1024 << "MB, label " << fs::getLabel()));
                fs::unmount();
                connected_ = false;
                instance_ = this;
            } else {
                info_->setText("No Card");
            }
        }

        ~DataSync() override {
            if (instance_ == this) {
                instance_ = nullptr;
                fs::mount(fs::Drive::SD);
            }
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
            // connect
            ASSERT(instance_->connected_ == false);
            instance_->connected_ = true;
            instance_->blocksRead_ = 0;
            instance_->blocksWrite_ = 0;
            LOG(LL_INFO, "MSC connected");
        }

        static void disconnect() {
            if (instance_ == nullptr)
                return;
            if (instance_->connected_) {
                instance_->connected_ = false;
                instance_ = nullptr;
            }
            LOG(LL_INFO, "MSC disconnected");
        }


    private:
        friend int32_t tud_msc_read10_cb(uint8_t, uint32_t, uint32_t, void *, uint32_t);
        friend int32_t tud_msc_write10_cb(uint8_t, uint32_t, uint32_t, uint8_t*, uint32_t);
        
        void onLoopStart() override {

            root_.flyIn();
        }

        void loop() override {
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                root_.flyOut();
                waitUntilIdle();
                exit();
            }
        }

        ui::Image * icon_ = nullptr;
        ui::Label * info_ = nullptr;
        ui::Label * status_ = nullptr;

        // true if cable is attached, false otherwise
        bool attached_ = false;
        // true if currently connected as MSC
        bool connected_ = false;
        uint32_t blocksRead_ = 0;
        uint32_t blocksWrite_ = 0;

        static inline DataSync * instance_ = nullptr;
    };
} // namespace rckid