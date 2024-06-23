#pragma once

#include "platform.h"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

/** The RCKid HTTP Bridge
 
    The bridge is the core part of both the base station and the WiFi cartridge. It provides a simple API for sending HTTP Requests and getting HTTP responses. The bridge is agnostic about the HTTP requests location, the decoding of which is left to the RCKid communicating with it. 
        
    The bridge itself is configurable via the same HTTP interface thanks to a simple web server it includes that utilizes AJAX requests to set various bridge features and a minimal human readable web interface built on top of those so that it can be configured from a browser. 

    From a security perspective, the bridge is extremely primitive. Security of the web server and configuration via it is provided by the WiFi network's security the bridge connects to (i.e. anyone who has access to the network has access to the bridge), while the security of the communication between RCKid itself and the bridge is delegated to the layer used. 
 */
class Bridge {
public:

    static void initialize() {
        LOG("Initializing the bridge...");
        initializeWiFi();
        initializeServer();
    }

    static void connect() {
        LOG("Scanning networks...");
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        wifiStatus_ = WiFiStatus::Connecting;
        cpu::delayMs(100);    
        WiFi.scanNetworksAsync(onWiFiScanDone);
    }

    static void loop() {
        MDNS.update();
        server_.handleClient();
    }

private:

    /** \name WiFi connection
     */

    enum class WiFiStatus {
        Off, 
        Connecting,
        Connected,
        Disconnected,
    }; // Bridge::WiFiStatus

    static inline WiFiEventHandler onWiFiConnected_;
    static inline WiFiEventHandler onWiFiIPAssigned_;
    static inline WiFiEventHandler onWiFiDisconnected_;

    volatile static inline WiFiStatus wifiStatus_;

    static void initializeWiFi() {
        LOG("Initializing WiFi...");
        onWiFiConnected_= WiFi.onStationModeConnected(onWiFiConnected);
        onWiFiIPAssigned_ = WiFi.onStationModeGotIP(onWiFiIPAssigned);
        onWiFiDisconnected_ = WiFi.onStationModeDisconnected(onWiFiDisconnected);

    }

    static void onWiFiScanDone(int n) {
        LOG(n << " networks found:");
        for (int i = 0; i < n; ++i)
            LOG("    " << WiFi.SSID(i) << " @" << WiFi.channel(i) << " (" << WiFi.RSSI(i) << "dbm)");
        // TODO see if there is a WiFi we want to use and connect to it (based on the saved priorities)

        LOG("Connecting to network " << WIFI_SSID);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        // we no longer need the networks list
        WiFi.scanDelete();
    }

    static void onWiFiConnected(WiFiEventStationModeConnected const & e) {
        LOG("WiFi: connected to " <<  e.ssid.c_str() << ", channel " << e.channel);
    }

    static void onWiFiIPAssigned(WiFiEventStationModeGotIP const & e) {
        LOG("WiFi: IP assigned: " << e.ip.toString().c_str() << ", gateway " << e.gw.toString().c_str());
        wifiStatus_ = WiFiStatus::Connected;
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
        if (wifiStatus_ != WiFiStatus::Connecting)
            wifiStatus_ = WiFiStatus::Disconnected;
    }

    /** \name HTTP and HTTPS clients
     */
    //@{
    static inline WiFiClient http_;
    static inline WiFiClientSecure https_;      
    //@}

    /** \name Web Server
     
        Simple web server that can be used to control the bridge. 
     */
    //@{

    static inline ESP8266WebServer server_{80};    

    static void initializeServer() {
        LOG("Initializing configuration web server...");
        if (!MDNS.begin("rckid-bridge"))
            LOG("  mDNS failed to initialize");
        server_.onNotFound(httpStaticHandler);
        //server_.on("/cmd", httpCommand);
        //server_.on("/status", httpStatus);
        //server_.on("/sdls", httpSDls);
        //server_.on("/sdrm", httpSDrm);
        //server_.on("/sd", httpSD);
        //server_.on("/sdUpload", HTTP_POST, httpSDUpload, httpSDUploadHandler);
        server_.begin();        
    }

    static void http404() {
        server_.send(404, "text/json","{ \"response\": 404, \"uri\": \"" + server_.uri() + "\" }");
    }

    static void httpStaticHandler() {
        http404();
    }
    
    //@}

}; // Bridge