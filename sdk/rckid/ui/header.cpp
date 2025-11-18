#include "../app.h"
#include "../wifi.h"
#include "../radio.h"

#include "style.h"
#include "form.h"
#include "header.h"


namespace rckid::ui {

    void Header::refresh() {
        // if we are running low on battery, or low on hearts and there is no header, we need to create one so that it can be displayed
        if (instance_ == nullptr) {
            // TODO
        }
        // if at this point we do not have an instance, then we do not need an instance, and there is nothing to update
        if (instance_ == nullptr)
            return;
        // now update stuff as we want


        // TODO determine if necessary to display
        if (instance_ == nullptr)
            return;
        instance_->update();
    }

    void Header::refreshStyle() {
        if (instance_ != nullptr)
            createPalette(instance_->palette_);
    }

    void Header::createPalette(uint16_t * palette) {
        // 0..15 is foreground color shading for fonts
        for (uint32_t i = 0; i < 16; ++i)
            palette[i] = Style::fg().withAlpha(255 * i / 15).toRaw();
        // 16..31 is accent color shading for fonts
        for (uint32_t i = 0; i < 16; ++i)
            palette[i + 16] = Style::accentFg().withAlpha(255 * i / 15).toRaw();
        // 32 is black + fg + accentFg
        palette[32] = 0;
        palette[33] = Style::fg().toRaw();
        palette[34] = Style::accentFg().toRaw();
        // 35 is black + fg + red 
        palette[35] = 0;
        palette[36] = Style::fg().toRaw();
        palette[37] = ColorRGB::Red().toRaw();
    }

        /** Updates the header. 
         
            Adds stuff like battery, etc. 
         */
    void Header::update() {
        using namespace assets;
/*

#if RCKID_ENABLE_STACK_PROTECTION
        // if tracking stack protection, display the memory usage and stack size
        text(0,0) << (memoryFree() / 1024) << '/' << StackProtection::maxSize() << ' ' << App::fps() << ' ';
        StackProtection::resetMaxSize();
#else 
        TinyDateTime now = timeNow();
        // if not tracking stack protection, just display the free memory
        text(0,0) << fillLeft(now.hour(), 2, '0') << ':' 
                    << fillLeft(now.minute(), 2, '0');
#endif
*/
        // update the system tray first 
        TinyDateTime t = timeNow();
        uint32_t x = 37; // battery takes 3 tiles, we have 40 in total
        if (powerCharging()) {
            at(39, 0).setPaletteOffset(PALETTE_ACCENT) = SYSTEM16_BATTERY_TOP_FULL;
            at(38, 0).setPaletteOffset(PALETTE_ACCENT) = SYSTEM16_BATTERY_CHARGING_FULL;
            at(37, 0).setPaletteOffset(PALETTE_ACCENT) = SYSTEM16_BATTERY_BOTTOM_FULL;
        } else if (powerUsbConnected()) {
            at(39, 0).setPaletteOffset(PALETTE_ACCENT) = SYSTEM16_BATTERY_TOP_FULL;
            at(38, 0).setPaletteOffset(PALETTE_ACCENT) = SYSTEM16_BATTERY_DC_FULL;
            at(37, 0).setPaletteOffset(PALETTE_ACCENT) = SYSTEM16_BATTERY_BOTTOM_FULL;
        } else {
            uint32_t vBatt = powerBatteryLevel();
            if (vBatt < 20 && t.time.second() % 2)
                at(37, 0).setPaletteOffset(PALETTE_RED) = SYSTEM16_BATTERY_BOTTOM_EMPTY;
            else if (vBatt < 30)
                at(37, 0).setPaletteOffset(PALETTE_RED) = SYSTEM16_BATTERY_BOTTOM_FULL;
            else
                at(37, 0).setPaletteOffset(PALETTE_ACCENT) = SYSTEM16_BATTERY_BOTTOM_FULL;
            at(38, 0).setPaletteOffset(PALETTE_ACCENT) = (vBatt < 65) ? SYSTEM16_BATTERY_MIDDLE_EMPTY : SYSTEM16_BATTERY_MIDDLE_FULL;               
            at(39, 0).setPaletteOffset(PALETTE_ACCENT) = (vBatt < 85) ? SYSTEM16_BATTERY_TOP_EMPTY : SYSTEM16_BATTERY_TOP_FULL;
        }
        // TODO now sd card
        // volume / headphones
        uint8_t vol = audioVolume();
        if (vol != 0)
            at(--x, 0).setPaletteOffset(PALETTE_ACCENT) = SYSTEM16_FILL_1 + (vol * 5 / 15);
        if (audioHeadphones()) {
            at(--x, 0).setPaletteOffset((vol == 0) ? PALETTE_RED + 1 : PALETTE_ACCENT) = SYSTEM16_HEADPHONES_RIGHT;
            at(--x, 0).setPaletteOffset((vol == 0) ? PALETTE_RED + 1 : PALETTE_ACCENT) = SYSTEM16_HEADPHONES_LEFT;
        } else {
            at(--x, 0).setPaletteOffset((vol == 0) ? PALETTE_RED + 1 : PALETTE_ACCENT) = SYSTEM16_SPEAKER_RIGHT;
            at(--x, 0).setPaletteOffset((vol == 0) ? PALETTE_RED + 1 : PALETTE_ACCENT) = SYSTEM16_SPEAKER_LEFT;
        }
        // check wifi
        if (WiFi::hasInstance()) {
            WiFi * wifi = WiFi::getOrCreateInstance();
            if (wifi->connected()) {
                at(--x, 0).setPaletteOffset(PALETTE_ACCENT) = SYSTEM16_WIFI_RIGHT;
                at(--x, 0).setPaletteOffset(PALETTE_ACCENT) = SYSTEM16_WIFI_LEFT;
            } else {
                at(--x, 0).setPaletteOffset(PALETTE_ACCENT + 1) = SYSTEM16_WIFI_RIGHT;
                at(--x, 0).setPaletteOffset(PALETTE_ACCENT + 1) = SYSTEM16_WIFI_LEFT;
            }
        }   
        // if the budget is below 10 minutes, display it as part of the header
        uint32_t b = budget();
        if (b < 600) {
            if ((b <= 120) && (timeNow().time.second() % 2 == 0)) {
                at(0,0).setPaletteOffset(PALETTE_RED) = ' ';
                at(1,0).setPaletteOffset(PALETTE_RED) = ' ';
            } else {
                at(0,0).setPaletteOffset(PALETTE_RED + 1) = SYSTEM16_HEART_LEFT;
                at(1,0).setPaletteOffset(PALETTE_RED + 1) = SYSTEM16_HEART_RIGHT;
            }
            uint32_t bm = b / 60;
            uint32_t bs = b % 60;
            at(2, 0).setPaletteOffset(0) = '0' + bm;
            at(3, 0).setPaletteOffset(0) = ':';
            at(4, 0).setPaletteOffset(0) = '0' + (bs / 10);
            at(5, 0).setPaletteOffset(0) = '0' + (bs % 10);
        // otherwise display the current time
        } else {
            App * app = App::currentApp();
            String const info = (app != nullptr) ? app->title() : "";
            for (uint32_t i = 0; i < x; ++i) {
                if (i >= info.size())
                    at(i, 0).setPaletteOffset(0) = ' ';
                else 
                    at(i, 0).setPaletteOffset(0) = info[i];
            }
        }
    }

    void ui::Header::renderIfRequired() {
        if (! refreshRequired_)
            return;
        refreshRequired_ = false;
        if (instance_ == nullptr)
            instance();
        instance_->refresh();
        Rect oldRegion = displayUpdateRegion();
        DisplayRefreshDirection oldDirection = displayRefreshDirection();
        displaySetRefreshDirection(DisplayRefreshDirection::ColumnFirst);
        displaySetUpdateRegion(Rect::XYWH(0, 0, RCKID_DISPLAY_WIDTH, 16));
        DoubleBuffer<uint16_t> buffer{16};
        Coord column = 319;
        instance_->renderRawColumn(column, buffer.front(), 0, 16);
        instance_->renderRawColumn(column, buffer.back(), 0, 16);
        displayUpdate(buffer.front(), 16, [&]() {
            if (--column < 0)
                return;
            buffer.swap();
            displayUpdate(buffer.front(), 16);
            if (column > 0)
                instance_->renderRawColumn(column - 1, buffer.back(), 0, 16);
        });
        displaySetRefreshDirection(oldDirection);
        displaySetUpdateRegion(oldRegion);
    }

    void ui::Header::renderRawColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) {
        if (FormWidget::bg_ != nullptr)
            FormWidget::bg_->renderColumn(column, buffer, starty, numPixels);
        else
            memset16(buffer, 0, numPixels);
        Tilemap::renderColumn(column, buffer, starty, numPixels);
    }



} // namespace rckid::ui