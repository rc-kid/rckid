#pragma once

#include "rckid/rckid.h"
#include "rckid/app.h"
#include "rckid/ui/timer.h"
#include "rckid/ui/ui.h"


namespace rckid {

    /** Simple pause dialog.
     * 
      */
    class Pause : public GraphicsApp<UITileEngine<>> {
    public:

        static void run() {
            Pause * p = new Pause{};
            p->loop();
        }

    protected:

        Pause():
            GraphicsApp{UITileEngine<>{26, 4, UITiles::Tileset, nullptr}}, 
            palette_{new ColorRGB[32] } {
            g_.setPalette(palette_);
            // initialize the palette
            for (uint8_t i = 0; i < 16; ++i) {
                palette_[i] = ColorRGB{(i << 4) + i, (i << 4) + i, (i << 4) + i};
                palette_[i + 16] = ColorRGB{(i << 4) + i, 0, 0};
            }
        }

        void update() override {
            App::update();
        }

        void draw() override {
            g_.text(0, 1) << "PAUSE";
            g_.text(0, 2) << "Press A to resume";
        }

        // palette
        ColorRGB * palette_; 

    }; // Pause

} // namespace rckid