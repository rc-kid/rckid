#include <rckid/audio/audio.h>
#include <rckid/ui/header.h>

namespace rckid::ui {

#define ADD_ICON(ICON, paletteOffset) \
    do { \
        auto icon = ICON; \
        x -= icon.size(); \
        update = instance_->contents_.setTileIcon(x, 0, icon, paletteOffset) || update; \
    } while (false)

    void Header::update() {

        TinyDateTime now = time::now();
        bool update = false;

        Coord x = 40;
        if (power::charging()) {
            ADD_ICON(TileIcon::batteryCharging(), PaletteOffsetRed);
        } else if (power::dcConnected()) {
            ADD_ICON(TileIcon::batteryDC(), PaletteOffsetGreen);
        } else {
            uint32_t batt = power::batteryLevel();
            if (batt >= 85) {
                ADD_ICON(TileIcon::batteryFull(), PaletteOffsetGreen);
            } else if (batt >= 65) {
                ADD_ICON(TileIcon::batteryTwoThirds(), PaletteOffsetGreen);
            } else if (batt >= 30) {
                ADD_ICON(TileIcon::batteryOneThird(), PaletteOffsetGreen);
            } else {
                ADD_ICON((now.time.second() % 2) ? TileIcon::batteryOneThird() : TileIcon::batteryEmpty(), PaletteOffsetRed);
            }
        }
        switch (audio::volume()) {
            case 1:
            case 2:
            case 3:
                ADD_ICON(TileIcon::fill1(), PaletteOffsetGreen);
                break;
            case 4:
            case 5:
                ADD_ICON(TileIcon::fill2(), PaletteOffsetGreen);
                break;
            case 6:
            case 7:
                ADD_ICON(TileIcon::fill3(), PaletteOffsetGreen);
                break;
            case 8:
            case 9:
            case 10:
                ADD_ICON(TileIcon::fill4(), PaletteOffsetGreen);
                break;
            case 11:
            case 12:
            case 13:
                ADD_ICON(TileIcon::fill5(), PaletteOffsetGreen);
                break;
            case 14:
            case 15: 
                ADD_ICON(TileIcon::fill6(), PaletteOffsetGreen);
                break;
            default:
                break;
        }
        if (audio::headphonesConnected())
           ADD_ICON(TileIcon::headphones(), audio::volume() == 0 ? (PaletteOffsetRed + 1) : PaletteOffsetGreen);
        else
           ADD_ICON(TileIcon::speaker(), audio::volume() == 0 ? (PaletteOffsetRed + 1) : PaletteOffsetGreen);

        uint32_t budget = pim::remainingBudget();
        if (budget != 0 && budget < 600) {
            update = true;
            TinyTime budgetTime{budget};
            instance_->contents_.setTileIcon(0, 0, TileIcon::heartEmpty(), PaletteOffsetRed + 1);
            instance_->contents_.text(2, 0)
                << alignRight(budgetTime.minute(), 2, '0')
                << ((now.time.second() % 2) ? ':' : ' ')
                << alignRight(budgetTime.second(), 2, '0');
        } else {
            instance_->contents().text(0, 0) 
                << alignRight(now.time.hour(), 2, '0')
                << ((now.time.second() % 2) ? ':' : ' ')
                << alignRight(now.time.minute(), 2, '0');
        }

        if (update) {
            instance_->show();
            if (instance_->remainingTicks_ < TicksToShowOnChange)
                instance_->remainingTicks_ = TicksToShowOnChange;
        }

    }


} // namespace rckid::ui