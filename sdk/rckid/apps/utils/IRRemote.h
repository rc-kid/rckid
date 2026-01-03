#pragma once

#include <platform.h>

#include "../../app.h"
#include "../../filesystem.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../ui/image.h"
#include "../../assets/icons_64.h"
#include "../../assets/fonts/OpenDyslexic64.h"
#include "../../audio/audio.h"
#include "../../utils/ini.h"
#include "../../pim.h"

namespace rckid {

    /** IR Remote
     
        The IR remote should multiple profiles that can be created and recorded. The recorded commands will then be replayed when the selected buttons are pressed.
     */
    class IRRemote : public ui::Form<void> {
    public:

        String name() const override { return "IRRemote"; }

        IRRemote():
            ui::Form<void>{}
        {
            icon_ = g_.addChild(new ui::Image{Icon{assets::icons_64::controller}});
            status_ = g_.addChild(new ui::Label{Rect::XYWH(0, 130, 320, 64), "TV"});
            status_->setFont(Font::fromROM<assets::OpenDyslexic64>());
            icon_->setTransparent(true);
            icon_->setPos(160 - icon_->width() / 2, 60);
            status_->setHAlign(HAlign::Center);
        }

    protected:

        static constexpr gpio::Pin IR_LED_PIN = 46;
        static constexpr gpio::Pin IR_RECEIVER_PIN = 12;

        void play(uint32_t * data, uint32_t length) {
#if (defined PLATFORM_RP2350)
            cpu::DisableInterruptsGuard g_;
#endif
            while (length-- > 0) {
                uint32_t x = *(data++);
                if (x > IR_ON) {
                    x -= IR_ON;
                    gpio::outputLow(IR_LED_PIN);
                } else {
                    gpio::setAsInput(IR_LED_PIN);
                }
                cpu::delayUs(x);
            }
        }

        void record() {
            // buffer to record the IR signal, 1024 transitions should be plenty for a typical remote command
            uint32_t * buffer = new uint32_t[1024];
            // now do the recording, the recording stops when second beginning is detected, of if we captured enough transitions
            // TODO do the recording
        }

        void update() override {
            ui::Form<void>::update();

            //if (btnPressed(Btn::B) || btnPressed(Btn::Down))
            //    exit();
        }

    private:
        static constexpr uint32_t IR_ON = 0x80000000;
        ui::Image * icon_;
        ui::Label * status_;

    }; // rckid::IRRemote
} // namespace rckid