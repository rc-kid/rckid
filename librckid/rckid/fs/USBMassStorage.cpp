
#include "filesystem.h"

#include "USBMassStorage.h"

namespace rckid {

    void USBMassStorage::onFocus() {
        App::onFocus();
        if (SD::ready()) {
            label_ = fs::getLabel();
            capacity_ = fs::getTotalCapacity();
            free_ = fs::getFreeCapacity();
            switch (fs::getFormat()) {
                case fs::Format::Unrecognized:
                    format_ = "unrecognized";
                    break;
                case fs::Format::FAT12:
                    format_ = "FAT12";
                    break;
                case fs::Format::FAT16:
                    format_ = "FAT16";
                    break;
                case fs::Format::FAT32:
                    format_ = "FAT32";
                    break;
                case fs::Format::EXFAT:
                    format_ = "EXFAT";
                    break;
            }
        } else {
            label_ = "---";
            format_ = "unrecognized";
            capacity_ = 0;
            free_ = 0;
        }
    }

    void USBMassStorage::onBlur() {
        // reset the USB so that we do not leak voltage on the VCC line through the USB ESD protection circuit 
        if (SD::status() == SD::Status::USB)
            SD::enableUSBMsc(false);
        App::onBlur();
    }

    void USBMassStorage::update() {
        // TODO the tud_init only works the first time as there is no tud_deinit. This should be added, for which we might need to fork the library
        //if (SD::status() != SD::Status::USB && dcPower())
        //    SD::enableUSBMsc(true);
        if (pressed(Btn::B))
            exit();
    }

}