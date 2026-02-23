#include <rckid/audio/audio.h>
#include <rckid/ui/header.h>

namespace rckid::ui {

    void Header::update() {
        Coord x = 40;
        x = instance_->addIcon(x, TileIcon::batteryFull(), PaletteOffsetGreen);
        switch (audio::volume()) {
            case 1:
            case 2:
            case 3:
                x = instance_->addIcon(x, TileIcon::fill1(), PaletteOffsetGreen);
                break;
            case 4:
            case 5:
                x = instance_->addIcon(x, TileIcon::fill2(), PaletteOffsetGreen);
                break;
            case 6:
            case 7:
                x = instance_->addIcon(x, TileIcon::fill3(), PaletteOffsetGreen);
                break;
            case 8:
            case 9:
            case 10:
                x = instance_->addIcon(x, TileIcon::fill4(), PaletteOffsetGreen);
                break;
            case 11:
            case 12:
            case 13:
                x = instance_->addIcon(x, TileIcon::fill5(), PaletteOffsetGreen);
                break;
            case 14:
            case 15: 
                x = instance_->addIcon(x, TileIcon::fill6(), PaletteOffsetGreen);
                break;
            default:
                break;
        }
        if (audio::headphonesConnected())
           x = instance_->addIcon(x, TileIcon::headphones(), audio::volume() == 0 ? (PaletteOffsetRed + 1) : PaletteOffsetGreen);
        else
           x = instance_->addIcon(x, TileIcon::speaker(), audio::volume() == 0 ? (PaletteOffsetRed + 1) : PaletteOffsetGreen);
    }


} // namespace rckid::ui