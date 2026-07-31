#pragma once

#include <rckid/rckid.h>

namespace rckid {

    /** Light sensor capability. 

     */
    class LightSensor {
    public:

        static LightSensor * instance();

        /** Returns the non-calibrated ambient light level. 
         
            This is the most primitive ligth sensor reading. It is unitless and the only thing that can be said about it is that 0 means absolute darkness and higher value is more light, 255 should ideally be considered as direct strong sunlight, i.e. the most light a child can be expected to be exposed to.
         */
        uint8_t ambientLight(); 
    }; // 

} // namespace rckid