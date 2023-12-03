#include "common/config.h"

#include "rckid.h"

namespace rckid {

    void initializeIO() {
        i2c_init(i2c0, RP_I2C_BAUDRATE); 
        // TODO serial 
    }

    void initializeAudio() {
        // one huge TODO
    }

} // namespace rckid