#include "USBMassStorage.h"

namespace rckid {

    void USBMassStorage::draw() {
        driver_.fill();
        driver_.text(0,0) << "USB MSC: " << numEvents_ << " events";
    }

}
