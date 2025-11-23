#pragma once

#include "rckid.h"
#include "task.h"
#include "utils/ini.h"
#include "filesystem.h"
#include "ui/header.h"

namespace rckid {

    /** WiFi manager
     
        The wifi manager defines a common API that backends should then implement to provide internet functionality. For now this means using the RM2 module for the mkIII hardware and using direct access for the fantasy console, but in the future other options are possible as well, such as using non-internet long distance radio to connect to a base station, etc. 

     */
    class WiFi : public Task {
    public:

        enum class Status {
            Off,
            Disconnected,
            Connecting,
            Connected
        }; 

        struct NetworkInfo {
            String ssid;
            String password;
        };

        enum class AuthMode {
            Open = 0,
            WPA_TKIP_PSK = 0x00020002, // WPA
            WPA2_AES_PSK = 0x00400004, // WPA2
            WPA2_MIXED_PSK = 0x00400006, // WPA2/WPA mixed
            WPA3_SAE_AES_PSK = 0x01000004, // WPA3 AES
            WPA3_WPA2_AES_PSK = 0x01400004 // WPA2/WPA2
        };

        /** Cannback function for wifi network scanning.
         */
        using ScanCallback = std::function<void(String && ssid, int16_t rssi, AuthMode authMode)>;

        using RequestCallback = std::function<void()>;

        static WiFi * getOrCreateInstance() {
            if (instance_ == nullptr)
                instance_ = initialize();
            return instance_; 
        }

        static bool hasInstance() { return instance_ != nullptr; }

        Status status() const; 

        /** Returns true if the wifi is enabled, false otherwise.
         */
        bool enabled() const { return status() != Status::Off; }

        /** Returns true if the wifi is connected to a network.
         */
        bool connected() const { return status() == Status::Connected; }

        /** Enables, or disables the WiFi.
            
            Enabling the wifi does no connect to any network, but simply enables the hardware. Must be the first function called.
         */
        void enable(bool value = true);
        
        /** Scans the available networks.
         */
        bool scan(ScanCallback callback);
        
        /** Connects to given WiFi network using specified authentification.
         */
        bool connect(String const & ssid, String const & password, AuthMode authMode); 

        /** Checks all available networks and connects to the most preferred one from those found. 
         
            Returns true if the scan was started successfully. The actual connection process is asynchronous and the connection result must be checked using connected().
         */
        bool connect() {
            if (knownNetworks_.empty())
                loadKnownNetworks();
            uint32_t index = knownNetworks_.size();
            AuthMode auth = AuthMode::Open;
            return scan([this, index, auth](String && ssid, [[maybe_unused]] int16_t rssi, AuthMode authMode) mutable {
                if (ssid.empty()) {
                    if (index < knownNetworks_.size())
                        connect(knownNetworks_[index].ssid, knownNetworks_[index].password, auth);
                    else
                        LOG(LL_INFO, "No known networks found");
                } else {
                    LOG(LL_INFO, "Found network: " << ssid);
                    for (uint32_t i = 0; i < index; ++i) {
                        if (knownNetworks_[i].ssid == ssid) {
                            index = i;
                            auth = authMode;
                            break;
                        }
                    }
                }
            });
        }

        /** Returns the current IP address as a single 32bit number. Returns 0 if no valid address/not connected.
         */
        uint32_t ipAddress();

        /** Simple HTTP request
         */
        bool http_get(char const * hostname, char const * path, RequestCallback callback);

        /** Simple HTTPS request
         */
        bool https_get(char const * hostname, char const * path, RequestCallback callback);

    protected:

        WiFi() = default;

        ~WiFi() override;

        void tick() override;

        Coord updateHeader(ui::Header & header, Coord endOffset) override {
            uint8_t paletteOffset;
            switch (status()) {
                case Status::Off:
                    return endOffset;
                case Status::Disconnected:
                    paletteOffset = ui::Header::PALETTE_RED + 1;
                    break;
                case Status::Connecting:
                    paletteOffset = ui::Header::PALETTE_FG;
                    break;
                case Status::Connected:
                    paletteOffset = ui::Header::PALETTE_ACCENT + 1;
                    break;
            }
            header.at(--endOffset, 0).setPaletteOffset(paletteOffset) = assets::SYSTEM16_WIFI_RIGHT;
            header.at(--endOffset, 0).setPaletteOffset(paletteOffset) = assets::SYSTEM16_WIFI_LEFT;
            return endOffset;
        }

        static WiFi * initialize();

    private:

        static constexpr char const * KNOWN_NETWORKS_PATH = "/wifi.ini";

        void loadKnownNetworks() {
            knownNetworks_.clear();
            ini::Reader reader{fs::fileRead(KNOWN_NETWORKS_PATH)};
            if (reader.eof())
                return;
            while (auto section = reader.nextSection()) {
                if (section == "network") {
                    NetworkInfo n;
                    while (auto kv = reader.nextValue()) {
                        if (kv->first == "ssid") {
                            n.ssid = kv->second;
                        } else if (kv->first == "password") {
                            n.password = kv->second;
                        } else {
                            LOG(LL_ERROR, "Unknown wifi network property " << kv->first);
                        }
                    }
                    knownNetworks_.push_back(n);
                } else {
                    LOG(LL_ERROR, "Invalid wifi section: " << section.value());
                }
                LOG(LL_INFO, "Known networks loaded: " << (uint32_t) knownNetworks_.size());
            }
        }

        ScanCallback scanCallback_;

        std::vector<NetworkInfo> knownNetworks_;

        static inline WiFi * instance_ = nullptr;

    }; 

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