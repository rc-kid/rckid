#pragma once

#include <ESP8266WiFi.h>

class wifi {
public:
    static void initialize() {
        onWiFiConnected_= WiFi.onStationModeConnected(onWiFiConnected);
        onWiFiIPAssigned_ = WiFi.onStationModeGotIP(onWiFiIPAssigned);
        onWiFiDisconnected_ = WiFi.onStationModeDisconnected(onWiFiDisconnected);
    }

    static void connect(char const * network, char const * password) {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        status_ = Status::Connecting;
        cpu::delayMs(100);    
        WiFi.begin(network, password);
    }

    static void disconnect() {

    }

private:

    static void onWiFiConnected(WiFiEventStationModeConnected const & e) {
        LOG("WiFi: connected to " <<  e.ssid.c_str() << ", channel " << e.channel);
    }

    static void onWiFiIPAssigned(WiFiEventStationModeGotIP const & e) {
        LOG("WiFi: IP assigned: " << e.ip.toString().c_str() << ", gateway " << e.gw.toString().c_str());
        status_ = Status::Connected;
    }

    /** TODO what to do when the wifi disconnects, but we did not initiate it? 
     
       reason 2
       reason 3?
       reason 201?

     */
    static void onWiFiDisconnected(WiFiEventStationModeDisconnected const & e) {
        LOG("WiFi: disconnected, reason:" << e.reason);
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
        // TODO is there autoreconnect? 
        // TODO maybe rescan and connect again? 
        if (status_ != Status::Connecting)
            status_ = Status::Disconnected;
    }

    enum class Status {
        Connecting,
        Connected,
        Disconnected,
    }; // wifi::Status

    volatile static inline Status status_ = Status::Disconnected;

    static inline WiFiEventHandler onWiFiConnected_;
    static inline WiFiEventHandler onWiFiIPAssigned_;
    static inline WiFiEventHandler onWiFiDisconnected_;

}; // class wifi