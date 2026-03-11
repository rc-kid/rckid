#pragma once

#include <rckid/rckid.h>
#include <rckid/task.h>
#include <rckid/string.h>
#include <rckid/ui/header.h>

namespace rckid {

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

        /** */
        class Connection {
        public:

            static constexpr int32_t IN_PROGRESS = -1;
            
            using Callback = std::function<void(Connection * conn, uint32_t size, uint8_t const * data)>;

            virtual ~Connection() { }

            virtual int32_t status() const = 0;

        protected:
            Connection(Callback cb): cb_{cb} {};

            Callback cb_;
        }; // WiFi::Connection

        /** Callback function for wifi network scanning.
         */
        using ScanCallback = std::function<void(String ssid, int16_t rssi, AuthMode authMode)>;

        Status status() const; 

        static WiFi * instance();

        ~WiFi() override;

        /** WiFi icon for the header. 
         */
        std::optional<std::pair<TileIcon, uint8_t>> headerIcon() const override {
            switch (status()) {
                case Status::Disconnected:
                    return std::make_pair(TileIcon::wifi(), ui::Header::PaletteOffsetRed + 1);
                case Status::Connecting:
                    return std::make_pair(TileIcon::wifi(), ui::Header::PaletteOffsetCyan + 1);
                case Status::Connected:
                    return std::make_pair(TileIcon::wifi(), ui::Header::PaletteOffsetBlue + 1);
                default:
                    UNREACHABLE;
                case Status::Off:
                    return std::nullopt;
            }
        }

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

        /** Returns the current IP address as a single 32bit number. Returns 0 if no valid address/not connected.
         */
        uint32_t ipAddress();

        /** Simple HTTP request
         */
        unique_ptr<Connection> http_get(String hostname, String path, Connection::Callback callback);

        /** Simple HTTPS request
         */
        unique_ptr<Connection> https_get(String hostname, String path, Connection::Callback callback);

    protected:

        void onTick() override;

        void releaseResources() override {
            enable(false);
            delete this;
        }

    }; // rckid::WiFi

} // namespace rckid