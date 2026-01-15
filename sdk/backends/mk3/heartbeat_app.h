#pragma once

#include <rckid/app.h>
#include <rckid/task.h>
#include <rckid/ui/form.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>
#include <rckid/assets/icons_64.h>
#include <rckid/assets/fonts/OpenDyslexic64.h>

#include "avr/src/avr_state.h"

namespace rckid {

    namespace io {
        extern AVRState avrState_;
    } // rckid::io

    /** Heartbeat app driver.
     */
    class HeartbeatApp : public ui::Form<void> {
    public:

        String name() const override { return "HeartbeatApp"; }

        HeartbeatApp():
            ui::Form<void>{}
        {
            icon_ = g_.addChild(new ui::Image{Icon{assets::icons_64::poo}});
            icon_->setTransparent(true);
            icon_->setPos(160 - icon_->width() / 2, 60);
            Task::runHeartbeatTask();
        }

    protected:

        void update() override {
            // if we are out of heartbeat mode, in which case the app should exit
            if (! io::avrState_.status.heartbeatMode()) {
                LOG(LL_INFO, "Exiting heartbeat app");
                displaySetBrightness(io::avrState_.brightness);
                exit();
            }
            // if the heartbeat task is done is done, in which case power off 
            else if (! Task::active()) {
                LOG(LL_INFO, "Heartbeat task done, powering off");
                powerOff();
            }
        }

    private:
        ui::Image * icon_;

    }; // rckid::HeartbeatApp
} // namespace rckid