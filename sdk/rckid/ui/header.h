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

     */
    class Header : public Tilemap<Tile<8, 16, Color16>> {
    public:
        Header() : Tilemap{40, 1, assets::System16, palette_} {
            fill(' ');
            // battery
            at(39, 0).setPaletteOffset(16) = 2;
            at(38, 0).setPaletteOffset(16) = 1;
            at(37, 0).setPaletteOffset(16) = 0;
            // sd card
            at(36, 0).setPaletteOffset(16) = 4;
            at(35, 0).setPaletteOffset(16) = 3;
        }

        static Header * instance() {
            if (instance_ == nullptr)
                instance_ = new Header{};
            return instance_;
        }

        /** Gathers the displayed data and updates the header. 
         
            If there is no header, but the displayed data are important, creates the header so that it can be displayed by the current app. 
         */
        static void refresh() {
            // TODO determine if necessary to display
            if (instance_ == nullptr)
                return;
            instance_->update();
        }

    protected:

        /** Updates the header. 
         
            Adds stuff like battery, etc. 
         */
        void update() override {
#if RCKID_ENABLE_STACK_PROTECTION
            // if tracking stack protection, display the memory usage and stack size
            text(0,0) << (memoryFree() / 1024) << '/' << memoryMaxStackSize() << ' ';
            memoryResetMaxStackSize();
#else 
            TinyDateTime now = timeNow();
            // if not tracking stack protection, just display the free memory
            text(0,0) << fillLeft(now.hour(), 2, '0') << ':' 
                      << fillLeft(now.minute(), 2, '0');
#endif
        }

        /** Unlike normal widgets,  */
        void renderRow(Coord row, uint16_t * buffer, Coord startx, Coord numPixels) {
            UNIMPLEMENTED;            
        }

    private:

        static inline Header * instance_ = nullptr;

        static constexpr uint16_t palette_[] = {
            // gray
            ColorRGB{0x00, 0x00, 0x00}.toRaw(), 
            ColorRGB{0x11, 0x11, 0x11}.toRaw(), 
            ColorRGB{0x22, 0x22, 0x22}.toRaw(), 
            ColorRGB{0x33, 0x33, 0x33}.toRaw(), 
            ColorRGB{0x44, 0x44, 0x44}.toRaw(), 
            ColorRGB{0x55, 0x55, 0x55}.toRaw(), 
            ColorRGB{0x66, 0x66, 0x66}.toRaw(), 
            ColorRGB{0x77, 0x77, 0x77}.toRaw(), 
            ColorRGB{0x88, 0x88, 0x88}.toRaw(), 
            ColorRGB{0x99, 0x99, 0x99}.toRaw(), 
            ColorRGB{0xaa, 0xaa, 0xaa}.toRaw(), 
            ColorRGB{0xbb, 0xbb, 0xbb}.toRaw(), 
            ColorRGB{0xcc, 0xcc, 0xcc}.toRaw(), 
            ColorRGB{0xdd, 0xdd, 0xdd}.toRaw(), 
            ColorRGB{0xee, 0xee, 0xee}.toRaw(), 
            ColorRGB{0xff, 0xff, 0xff}.toRaw(), 
            0, 
            ColorRGB{0xff, 0xff, 0xff}.toRaw(), 
            ColorRGB{0x00, 0xff, 0x00}.toRaw(),
            0x4444, 
            0x5555, 
            0x6666, 
            0x7777, 
            0x8888, 
            0x9999, 
            0xaaaa, 
            0xbbbb, 
            0xcccc, 
            0xdddd, 
            0xeeee, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
        };
    }; // rckid::ui::Header

} // namespace rckid::ui