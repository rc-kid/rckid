#pragma once

#include "../../app.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../ui/image.h"

namespace rckid {

    /** WiFi Manager.

        Manages the WiFi network connections. While the actual wifi functionality is handled by the wifi task, this app provides a simple interface for getting the WiFi state and for managing the connections & settings.
     */
    class WiFiManager : public ui::Form<void> {
    public:

        String name() const override { return "WiFiManager"; }

        WiFiManager(): 
            ui::Form<void>{}
        {
            icon_ = g_.addChild(new ui::Image{160 - 32, 60, Icon{assets::icons_64::wifi}});
        }

    protected:

        void update() override {
            ui::Form<void>::update();
            if (btnPressed(Btn::B))
                exit();
        }

    private:

        ui::Image * icon_;

    }; // rckid::Alarm
}