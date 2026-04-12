#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/image.h>
#include <rckid/apps/launcher.h>

#include <assets/icons_64.h>

namespace rckid {

    /** Very basic color picker.  */
    class ColorPicker : public ui::App<Color> {
    public:
        
        String name() const override { return "ColorPicker"; }



    }; // rckid::ColorPicker

} // namespace rckid 