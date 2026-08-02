#pragma once

#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>


#include <rckid/capabilities/jacdac.h>

#include <assets/Iosevka24.h>
#include <assets/icons_64.h>

namespace rckid {

    /** Very simple Jacdac service that exposes the internal accelerometer over Jacdac bus as a Jacdac device. 
     
        Frame: 

        86e3 0c 00 502b6e9c4edaa3b8 080000002f010b000904141f
        crc  s  f  devide id        data
                                    08 00 0000 2f010b000904141f
                                    s  id cmd  


        332d 10 00 502b6e9c4edaa3b8 0c01011100000800000008000000f8ff
        crc  s  f  device id        data
                                    0c 01 0111 00000800000008000000f8ff
                                    s  id cmd
     */
    class JacdacService : public ui::App<void> {
    public:

        String name() const override { return "Jacdac Service"; }

        JacdacService() {
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

        ~JacdacService() override {
            if (jacdac_ != nullptr) {
                jacdac_->disable();
            }
        }

    private:
        
        void onLoopStart() override {

            root_.flyIn();
        }

        /** Converts the accelerometer values to 12.20 fixed-point format 
         */
        int16_t toInt12_20(int32_t value) {
            // the values from the accelerometer are 16384 per g, the Jacdac IMU service units are in g's so we need to convert the values to 12.20 fixed-point format, which is 1048576 per g
            return static_cast<int16_t>(value * 1048576 / 16384);
        }

        void loop() override {
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                root_.flyOut();
                waitUntilIdle();
                exit();
            }
            if (jacdac_) {
                
                // every 500ms, send the service packet
                if (tickCounter_ % 30 == 0) {
                    uint8_t f[] = {0x86, 0xe3, 0x0c, 0x00, 0x50, 0x2b, 0x6e, 0x9c, 0x4e, 0xda, 0xa3, 0xb8, 0x08, 0x00, 0x00, 0x00, 0x2f, 0x01, 0x0b, 0x00, 0x09, 0x04, 0x14, 0x1f};
                    jacdac_->sendFrame(f, sizeof(f));
                    cmdCount_++;
                }
                // a bit faster, send the update of the forces
                if (tickCounter_ % 10 == 5) {
                    uint8_t data[] = {0x33, 0x2d, 0x10, 0x00, 0x50, 0x2b, 0x6e, 0x9c, 0x4e, 0xda, 0xa3, 0xb8, 0x0c, 0x01, 0x01, 0x11, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0xf8, 0xff};
                    Jacdac::Frame  * f = reinterpret_cast<Jacdac::Frame *>(data);
                    Jacdac::Packet * p = reinterpret_cast<Jacdac::Packet *>(f->data);
                    int16_t * acc = reinterpret_cast<int16_t *>(p->payload);
                    Point3D a = accel();
                    acc[0] = toInt12_20(a.x);
                    acc[1] = toInt12_20(a.y);
                    acc[2] = toInt12_20(a.z);
                    f->updateCrc();
                    updateCount_++;

                    jacdac_->sendFrame(data, sizeof(data));
                }
                tickCounter_++;
                /*
                info_->setText(STR("Errors: " << jacdac_->errors << ", status: " << jacdac_->rxStatus));
                */
                status_->setText(STR("Con:" << cmdCount_ << ", updates: " << updateCount_));
            } else {
                status_->setText("No Jacdac");
            }
        }

        ui::Image * icon_ = nullptr;
        ui::Label * info_ = nullptr;
        ui::Label * status_ = nullptr;

        Jacdac * jacdac_ = nullptr;

        uint64_t deviceId_ = 0x1122334455667788ul;
        uint32_t tickCounter_ = 0;
        uint8_t cmdIndex_ = 0;
        uint32_t cmdCount_ = 0;
        uint32_t updateCount_ = 0;
    };
} // namespace rckid