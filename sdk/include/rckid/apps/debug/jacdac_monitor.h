#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>


#include <rckid/capabilities/jacdac.h>

#include <assets/Iosevka24.h>
#include <assets/icons_64.h>

namespace rckid {

    /** Super simple Jacdac bus monitor for debugging purposes.
     */
    class JacdacMonitor : public ui::App<void> {
    public:

        String name() const override { return "Jacdac Monitor"; }

        JacdacMonitor() {
            using namespace ui;
            jacdac_ = Jacdac::instance();
            icon_ = addChild(new Image())
                << SetRect(Rect::XYWH(0, 60, 320, 64))
                << SetBitmap(assets::icons_64::jacdac);
            info_ = addChild(new Label())
                << SetRect(Rect::XYWH(0, 140, 320, 24))
                << SetFont(assets::Iosevka24)
                << SetHAlign(HAlign::Center);
            status_ = addChild(new Label())
                << SetRect(Rect::XYWH(0, 160, 320, 24))
                << SetText("Disconnected")
                << SetFont(assets::Iosevka24)
                << SetHAlign(HAlign::Center);

            if (jacdac_ != nullptr) {
                jacdac_->enable();
            }
        }

        ~JacdacMonitor() override {
            if (jacdac_ != nullptr) {
                jacdac_->disable();
            }
        }

    private:
        
        void onLoopStart() override {

            root_.flyIn();
        }

        void loop() override {
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                root_.flyOut();
                waitUntilIdle();
                exit();
            }
            if (jacdac_) {
                info_->setText(STR("Errors: " << jacdac_->errors << ", status: " << jacdac_->rxStatus));
                status_->setText(STR("Rx (" << jacdac_->receivedPackets << ", Bytes: " << jacdac_->receivedBytes << ")"));
            } else {
                status_->setText("No Jacdac");
            }
        }

        ui::Image * icon_ = nullptr;
        ui::Label * info_ = nullptr;
        ui::Label * status_ = nullptr;

        Jacdac * jacdac_ = nullptr;
    };
} // namespace rckid