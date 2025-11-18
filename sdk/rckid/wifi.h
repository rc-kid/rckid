#pragma once

#include "rckid.h"
#include "task.h"

namespace rckid {

    /** WiFi manager
     
        The wifi manager defines a common API that backends should then implement to provide internet functionality. For now this means using the RM2 module for the mkIII hardware and using direct access for the fantasy console, but in the future other options are possible as well, such as using non-internet long distance radio to connect to a base station, etc. 

     */
    class WiFi {
    public:

        enum class AuthMode {
            Open = 0,
            WPA_TKIP_PSK = 0x00020002, // WPA
            WPA2_AES_PSK = 0x00400004, // WPA2
            WPA2_MIXED_PSK = 0x00400006, // WPA2/WPA mixed
            WPA3_SAE_AES_PSK = 0x01000004, // WPA3 AES
            WPA3_WPA2_AES_PSK = 0x01400004 // WPA2/WPA2
        };

        using ScanCallback = std::function<bool(String && ssid, int16_t rssi, AuthMode authMode)>;

        /** Returns the singleton instance of the WiFi manager. 
         
            If WiFi is not available on the current hardware, returns nullptr and this *must* be checked before use. 
         */
        static WiFi * instance() { 
            if (instance_ == nullptr)
                instance_ = initialize();
            return instance_; 
        }

        static bool hasInstance() {
            return instance_ != nullptr;
        }

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

        bool scan(ScanCallback callback);

        void connect(String const & ssid, String const & password); 

    private:

        friend void initialize(int argc, char const * argv[]);

        static WiFi * initialize();

        ~WiFi();

        ScanCallback scanCallback_;

        static inline WiFi * instance_ = nullptr;
    }; // class rckid::WiFi

    inline Writer & operator << (Writer & w, WiFi::AuthMode const & mode) {
        switch (mode) {
            case WiFi::AuthMode::Open:
                w << "Open";
                break;
            case WiFi::AuthMode::WPA_TKIP_PSK:
                w << "WPA";
                break;
            case WiFi::AuthMode::WPA2_AES_PSK:
                w << "WPA2";
                break;
            case WiFi::AuthMode::WPA2_MIXED_PSK:
                w << "WPA2/WPA";
                break;
            case WiFi::AuthMode::WPA3_SAE_AES_PSK:
                w << "WPA3";
                break;
            case WiFi::AuthMode::WPA3_WPA2_AES_PSK:
                w << "WPA2/WPA3";
                break;
            default:
                w << "unknown(" << static_cast<uint32_t>(mode) << ")";
                break;
        }
        return w;
    }

} // namespace rckid