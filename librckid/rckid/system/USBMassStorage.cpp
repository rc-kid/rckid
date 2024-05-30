
#include "tusb.h"
#include "hardware/structs/usb.h"


#include "USBMassStorage.h"


namespace rckid {

    void USBMassStorage::onFocus() {
        App::onFocus();
        SD::enableUsbMsc(true);
    }

    void USBMassStorage::onBlur() {
        // reset the USB so that we do not leak voltage on the VCC line through the USB ESD protection circuit 
        if (connected_) {
            connected_ = false;
            memset(usb_hw, 0, sizeof(*usb_hw));
        }
        // TODO: remount the SD card
        App::onBlur();
        SD::enableUsbMsc(false);
    }

    void USBMassStorage::update() {
        // TODO the tud_init only works the first time as there is no tud_deinit. This should be added, for which we might need to fork the library
        if (!connected_ && dcPower()) {
            connected_ = true;
            tud_init(BOARD_TUD_RHPORT);
        }
        if (pressed(Btn::B))
            exit();
    }


}