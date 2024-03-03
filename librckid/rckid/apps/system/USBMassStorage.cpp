#include "USBMassStorage.h"

#include "fonts/Iosevka_Mono6pt7b.h"

namespace rckid {

    void USBMassStorage::update() {
        yield();
    }

    void USBMassStorage::draw() {
        fb_.fill();
        fb_.text(0,0) << "USB MSC: " << numEvents_ << " events";
    }

}

using namespace rckid;
