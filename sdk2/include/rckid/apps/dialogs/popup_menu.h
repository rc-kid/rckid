#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/menu.h>

namespace rckid {


    class PopupMenu : public ui::App<ui::MenuItem*> {
    public:

        String name() const override { return "PopupMenu"; }

        PopupMenu(ui::Menu * menu):
            menu_{menu} 
        {

        }

    private:

        void loop() override {

        }

        ui::Menu * menu_;

    }; // rckid::PopupMenu


}