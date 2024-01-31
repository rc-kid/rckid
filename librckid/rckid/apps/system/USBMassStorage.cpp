#include "USBMassStorage.h"

#include "fonts/Iosevka_Mono6pt7b.h"

namespace rckid {

    void USBMassStorage::update() {
        yield();
    }

    void USBMassStorage::draw() {
        Renderer & r = renderer();
        r.fill();
        r.text(0,0) << "USB MSC: " << numEvents_ << " events";
    }

}

using namespace rckid;
