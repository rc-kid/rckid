#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>

#include <assets/OpenDyslexic64.h>
#include <assets/icons_64.h>

#if (defined PLATFORM_RP2350)
#include <pico/bootrom.h>
#endif

namespace rckid {

    /** RP Bootloader.
     */
    class Bootloader : public ui::App<void> {
    public:

        String name() const override { return "Bootloader"; }

        Bootloader() {
            using namespace ui;
            icon_ = addChild(new Image())
                << SetRect(Rect::XYWH(0, 60, 320, 64))
                << SetBitmap(assets::icons_64::microchip);
            text_ = addChild(new Label())
                << SetRect(Rect::XYWH(0, 130, 320, 64))
                << SetText("USB Bootloader")
                << SetFont(assets::OpenDyslexic64)
                << SetHAlign(HAlign::Center);
        }

    protected:

        void onLoopStart() override {
            root_.flyIn();
        }

        void loop() override {
            using namespace ui;
            App<void>::loop();
            if (root_.idle()) {
                // TODO tell AVR to show the uploader lights (and tell it we are in bootloader mode so no pings etc.)
#if (defined PLATFORM_RP2350)
                rom_reset_usb_boot(0, 0);
#endif
            }
        }

    private:
        ui::Image * icon_;
        ui::Label * text_;

    }; // RPBootloader
} // namespace rckid