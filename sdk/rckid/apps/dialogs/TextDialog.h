#pragma once

#include "../../ui/form.h"
#include "../../ui/tilemap.h"
#include "../../assets/tiles/System24.h"

namespace rckid {

    
    /** A simple dialog app that returns text written by the user via an on screen keyboard.
      
        Uses a simple tilemap and a sprite for the selection.  
     */
    class TextDialog : public ui::App<std::string> {
    public:
        TextDialog():
            ui::App<std::string>{} {
                using namespace ui;
                tileMap_ = new ui::Tilemap<ui::Tile<12, 24, Color256>>{26, 4, assets::System24, palette_};
            }

    private:
        ui::Tilemap<Tile<12,24,Color256>> * tileMap_; 

    }; // rckid::TextDialog

} // namespace rckid