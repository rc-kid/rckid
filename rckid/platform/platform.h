#pragma once
#include <stdint.h>

#include <common/errors.h>

/** On top of the RPi Pico SDK macros (see https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#macros48), RCKid defines a few extra macros of its own to decorate functions:
 */

#define __noreturn(FUN_NAME) __attribute__((noreturn)) FUN_NAME

namespace rckid {
    /** Raises a fatal error and shows a blue screen of death. 
     
        The function can be called from either core, but is always serviced by core 0. It suspends all execution and shows the blue screen of death with some basic information about the error, after which enters a forever loop. To recover from the error, the device has to be reset with the Home key long press. 

        The function shoudl not be called directly, but instead through the various error macros, such as FATAL_ERROR, ASSUME, etc. Those macros also set the appropriate error metadata such as line and file name, etc. 

        Internally, the fatal error works by triggering TIMER0 interrupt, the ISR is masked to core 0, immediately halts core 1 and displays the blue screen of deatch from the ISR itself. The TIMER0 interrupt is given the highest priority (and since it has the lowest IRQ number), it cannot be preempted by any other interrupt. 
       
     */
    //@{
    void __noreturn(fatalError)(uint32_t code, uint32_t line = 0, char const * file = nullptr);

    inline void __noreturn(fatalError)(Error code, uint32_t line = 0, char const * file = nullptr) {
        fatalError(static_cast<uint32_t>(code), line, file);
    }
    //@}
}

// platform definition overrides (those are defined in rckid.h)
#define ASSERT(...) do { if (!(__VA_ARGS__)) FATAL_ERROR(::rckid::Error::AssertFailure); } while (false)
#define UNIMPLEMENTED FATAL_ERROR(::rckid::Error::Unimplemented)
#define UNREACHABLE FATAL_ERROR(::rckid::Error::Unreachable)

#define FATAL_ERROR(ERR) ::rckid::fatalError(ERR, __LINE__, __FILE__)


// now include the real platform, which is either RP2040, or mock

#if (defined RCKID_MOCK)
    #include "platform/mock/platform.h"
#else
    #include "platform/rp2040/platform.h"
#endif

#include <platform/utils/writer.h>

// include the configuration macros so that all rckid projects can share them
#include <common/config.h>

namespace rckid {
    /** Returns a writes to the USB virtual COM port.
     */
    Writer writeToSerial();
}
