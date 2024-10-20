#pragma once

#include "../rckid.h"
#include "../app.h"
#include "ui.h"


namespace rckid {

    /** Simple alert dialog
     * 
      */
    class Alert : public GraphicsApp<UITileEngine<>> {
    public:

        static void run(char const * title, char const * message) {
            Alert a{title, message};
            a.loop();
        }

    protected:

        Alert(char const * title, char const * message):
            GraphicsApp{UITileEngine<>{26, 4, UITiles::Tileset, nullptr}}, 
            palette_{new ColorRGB[32] }, 
            title_{title}, 
            message_{message} {
            g_.setPalette(palette_);
            // initialize the palette
            for (uint8_t i = 0; i < 16; ++i) {
                palette_[i] = ColorRGB{(i << 4) + i, (i << 4) + i, (i << 4) + i};
                palette_[i + 16] = ColorRGB{(i << 4) + i, 0, 0};
            }
        }

        void update() override {
            App::update();
            if (btnPressed(Btn::A))
                exit();
        }

        void draw() override {
            g_.text(0, 1) << title_;
            g_.text(0, 2) << message_;
        }

        // palette
        ColorRGB * palette_; 
        char const * title_;
        char const * message_;

    }; // Alert

} // namespace rckid