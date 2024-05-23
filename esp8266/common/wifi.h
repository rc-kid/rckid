#pragma once

#include <string>

/** WiFi connection manager. 
 */
class WiFi {
public:

    void enable() {

    }

    void disable() {

    }

private:

    /** WiFi initialization routine. 
     */
    static void initializeWiFi() {
        //LOG("Initializing WiFi...");
        wifiConnectedHandler_ = WiFi.onStationModeConnected(onWiFiConnected);
        wifiIPAssignedHandler_ = WiFi.onStationModeGotIP(onWiFiIPAssigned);
        wifiDisconnectedHandler_ = WiFi.onStationModeDisconnected(onWiFiDisconnected);
    }

    /** Connects to the WiFi. 
     */
    static void connectWiFi() {
        //LOG("WiFi: Scanning networks...");
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        state_.setWiFiStatus(WiFiStatus::Connecting);
        WiFi.scanNetworksAsync([](int n) {
            LOG("WiFi: Networks found: %i", n);
            File f = SD.open(WIFI_SETTINGS_FILE, FILE_READ);
            if (f) {
                StaticJsonDocument<1024> json;
                if (deserializeJson(json, f) == DeserializationError::Ok) {
                    for (JsonVariant network : json["networks"].as<JsonArray>()) {
                        for (int i = 0; i < n; ++i) {
                            if (WiFi.SSID(i) == network["ssid"]) {
                                LOG("WiFi: connecting to %s, rssi: %i, channel: %i", WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
                                send(msg::SetWiFiStatus{WiFiStatus::Connecting});
                                WiFi.begin(network["ssid"].as<char const *>(), network["password"].as<char const *>());
                                // so that we do not start the access point just yet
                                WiFi.scanDelete();
                                f.close();
                                return;
                            }
                        }
                    }
                } else {
                    LOG("");
                }
                f.close();
            } else {
                LOG("WiFi: No %s found", WIFI_SETTINGS_FILE);
            }
            WiFi.scanDelete();
            // if no networks were recognized, start AP
            startWiFiAP();
        });
    }

    static void startWiFiAP() {
        File f = SD.open(WIFI_SETTINGS_FILE, FILE_READ);
        if (f) {
            StaticJsonDocument<1024> json;
            if (deserializeJson(json, f) == DeserializationError::Ok) {
                char const * ssid = json["ap"]["ssid"];
                char const * pass = json["ap"]["pass"];
                if (ssid == nullptr || ssid[0] == 0) {
                    ssid = DEFAULT_AP_SSID;
                    if (pass == nullptr || pass[0] == 0)
                        pass = DEFAULT_AP_PASSWORD;
                }
                LOG("Initializing soft AP, ssid %s, password %p", ssid, pass);
                LOG("    own ip: 10.0.0.1");
                LOG("    subnet: 255.255.255.0");
                IPAddress ip{10,0,0,1};
                IPAddress subnet{255, 255, 255, 0};
                WiFi.softAPConfig(ip, ip, subnet);
                if (!WiFi.softAP(ssid, pass)) {
                    // TODO error
                } else {
                    state_.setWiFiStatus(WiFiStatus::AP);
                    send(msg::SetWiFiStatus{WiFiStatus::AP});
                }
            }
            f.close();
        }
    }

    static void disconnectWiFi() {
        LOG("WiFi: Disconnecting");
        WiFi.mode(WIFI_OFF);
        WiFi.forceSleepBegin();
        yield();
        state_.setWiFiStatus(WiFiStatus::Off);
        send(msg::SetWiFiStatus{WiFiStatus::Off});
    }

    static void onWiFiConnected(WiFiEventStationModeConnected const & e) {
        LOG("WiFi: connected to %s, channel %u", e.ssid.c_str(), e.channel);
    }

    static void onWiFiIPAssigned(WiFiEventStationModeGotIP const & e) {
        LOG("WiFi: IP assigned: %s, gateway: %s", e.ip.toString().c_str(), e.gw.toString().c_str());
        state_.setWiFiStatus(WiFiStatus::Connected);
        send(msg::SetWiFiStatus{WiFiStatus::Connected});
        // wait 7 seconds and then check time (checking time here usually led to errors)
        status_.checkTime = 7;
    }

    /** TODO what to do when the wifi disconnects, but we did not initiate it? 
     
       reason 2
       reason 3?
       reason 201?

     */
    static void onWiFiDisconnected(WiFiEventStationModeDisconnected const & e) {
        LOG("WiFi: disconnected, reason: %u", e.reason);
        switch (e.reason) {
            case REASON_UNSPECIFIED:
            case REASON_AUTH_EXPIRE:
            case REASON_AUTH_LEAVE:
            case REASON_ASSOC_EXPIRE:
            case REASON_ASSOC_TOOMANY:
            case REASON_NOT_AUTHED:
            case REASON_NOT_ASSOCED:
            case REASON_ASSOC_LEAVE:
            case REASON_ASSOC_NOT_AUTHED:
            case REASON_DISASSOC_PWRCAP_BAD:
            case REASON_DISASSOC_SUPCHAN_BAD:
            case REASON_IE_INVALID:
            case REASON_MIC_FAILURE:
            case REASON_4WAY_HANDSHAKE_TIMEOUT:
            case REASON_GROUP_KEY_UPDATE_TIMEOUT:
            case REASON_IE_IN_4WAY_DIFFERS:
            case REASON_GROUP_CIPHER_INVALID:
            case REASON_PAIRWISE_CIPHER_INVALID:
            case REASON_AKMP_INVALID:
            case REASON_UNSUPP_RSN_IE_VERSION:
            case REASON_INVALID_RSN_IE_CAP:
            case REASON_802_1X_AUTH_FAILED:
            case REASON_CIPHER_SUITE_REJECTED:
            case REASON_BEACON_TIMEOUT:
            case REASON_NO_AP_FOUND:
            case REASON_AUTH_FAIL:
            case REASON_ASSOC_FAIL:
            case REASON_HANDSHAKE_TIMEOUT:
                break;
        }
        if (state_.wifiStatus() != WiFiStatus::Connecting) {
            state_.setWiFiStatus(WiFiStatus::Off);
            send(msg::SetWiFiStatus{WiFiStatus::Off});
        }
    }

    static inline WiFiEventHandler wifiConnectedHandler_;
    static inline WiFiEventHandler wifiIPAssignedHandler_;
    static inline WiFiEventHandler wifiDisconnectedHandler_;


    static inline bool busy_ = false;
    static inline bool enabled_ = false;
    static inline std::string ssid_; 

}; // WiFi