#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>

#include <assets/OpenDyslexic64.h>
#include <assets/icons_64.h>

namespace rckid {

    /** Step counter.
     
        A very simple step counter app that shows the current step count on the screen. 

        TODO: daily stats & history, goals, etc.
     */
    class Steps : public ui::App<void> {
    public:
        Steps() {
            using namespace ui;
            addChild(new Image())
                << SetRect(Rect::XYWH(0, 60, 320, 64))
                << SetBitmap(assets::icons_64::footprint);
            addChild(new Label())
                << SetRect(Rect::XYWH(0, 130, 320, 64))
                << SetText("Steps Counter")
                << SetFont(assets::OpenDyslexic64)
                << SetHAlign(HAlign::Center);
        }

    private:

        // step counter
        ui::Label * steps_ = nullptr;

    }; // rckid::Steps
    

} // namespace rckid