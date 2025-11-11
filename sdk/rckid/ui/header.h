#pragma once

#include "widget.h"
#include "../graphics/tile.h"
#include "../assets/tiles/System16.h"
#include "tilemap.h"

namespace rckid::ui {

    /** Application hedear. 
     
        The header displays information such as current time, battery status, sound volume, signal strength, etc. Icons and displayed from right whle text is displayed from left. 

        - charging
        - battery status
        - headphones
        - volume
        - recording
        - wifi
        - bluetooth
        - radio

        System tray, which is the right part of the header that shows the following system icons, from right to left:

        - battery status
            - lightning bolt inside if charging
            - checkmark inside if full & DC power on
        - SD card (error if inserted, but not readable)
        - Speaker / headphones icon & volume level
        - WiFi icon 
            - default color when enabled
            - accent color when connected

        The rest (from left to end of system tray beginning) can be filled with arbitrary information (mostly text). 

        To display all the things, the header uses a 40x1 tilemap with 8x16 tiles and a palette, that follows this layoput: 

            0 - 15 = normal color shading
            16 - 31 = normal color with 1 accent color,
            32 - 47 = normal color with 2 accent colors,
            48 - 63 = normal color with 3 accent colors
            and so on
        
     */
    class Header : public Tilemap<Tile<8, 16, Color16>> {
    public:
        Header() : Tilemap{40, 1, assets::System16, palette_} {
            createPalette(palette_);
            fill(' ');
        }

        static Header * instance() {
            if (instance_ == nullptr) {
                instance_ = new Header{};
                refresh();
            }
            return instance_;
        }

        /** Gathers the displayed data and updates the header. 
         
            If there is no header, but the displayed data are important, creates the header so that it can be displayed by the current app. This is called by the application's second tick handler.
         */
        static void refresh();

        static bool refreshRequired() { return refreshRequired_; }

        static void requireRefresh() { refreshRequired_ = true; }

        static void refreshStyle();

        static void createPalette(uint16_t * palette);

        static void renderIfRequired(); 

    protected:

        /** Updates the header. 
         
            Adds stuff like battery, etc. 
         */
        void update() override;

        void draw() override {
            Tilemap::draw();
            refreshRequired_ = false;
        }

        /** Unlike normal widgets,  */
        void renderRow([[maybe_unused]] Coord row, [[maybe_unused]] uint16_t * buffer, [[maybe_unused]] Coord startx, [[maybe_unused]] Coord numPixels) {
            UNIMPLEMENTED;            
        }

        void renderRawColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels);

    private:

        static constexpr uint32_t PALETTE_FG = 0;
        static constexpr uint32_t PALETTE_ACCENT_FG = 16;
        static constexpr uint32_t PALETTE_ACCENT = 32;
        static constexpr uint32_t PALETTE_RED = 35;

        static inline Header * instance_ = nullptr;
        static inline bool refreshRequired_ = false;

        uint16_t palette_[256]; 
    }; // rckid::ui::Header

} // namespace rckid::ui