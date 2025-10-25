#pragma once

#include "rckid.h"

namespace rckid {

    /** WiFi manager
     
        The wifi manager defines a common API that backends should then implement to provide internet functionality. For now this means using the RM2 module for the mkIII hardware and using direct access for the fantasy console, but in the future other options are possible as well, such as using non-internet long distance radio to connect to a base station, etc. 

     */
    class WiFi {
    public:
        /** Returns the singleton instance of the WiFi manager. 
         
            If WiFi is not available on the current hardware, returns nullptr and this *must* be checked before use. 
         */
        static WiFi * instance() { return instance_; }

        /** Enables the WiFi. 
            
            This does not connect to any network, but simply enables the hardware. Must be the first function called.
         */
        void enable();

        /** Disables the WiFi.
         
            Turns the module off, but does not deinitialize the driver.
         */
        void disable();

        /** Returns true if the wifi is enabled, false otherwise.
         */
        bool enabled() const;

        /** Returns true if the wifi is connected. 
         */
        bool connected() const;

    private:

        friend void initialize(int argc, char const * argv[]);

        static void initialize();

        ~WiFi();

        static inline WiFi * instance_ = nullptr;
    }; // class rckid::WiFi


} // namespace rckid