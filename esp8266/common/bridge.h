#pragma once

#include "platform.h"

#include <ESP8266WiFi.h>


/** The RCKid HTTP Bridge
 
    The bridge is the core part of both the base station and the WiFi cartridge. It provides a simple API for sending HTTP Requests and getting HTTP responses. The bridge is agnostic about the HTTP requests location, the decoding of which is left to the RCKid communicating with it. 
        
    The bridge itself is configurable via the same HTTP interface thanks to a simple web server it includes that utilizes AJAX requests to set various bridge features and a minimal human readable web interface built on top of those so that it can be configured from a browser. 

    From a security perspective, the bridge is extremely primitive. Security of the web server and configuration via it is provided by the WiFi network's security the bridge connects to (i.e. anyone who has access to the network has access to the bridge), while the security of the communication between RCKid itself and the bridge is delegated to the layer used. 
 */
class Bridge {
public:
    static void initialize() {
        LOG("Initializing the bridge...");
        LOG("Enabling WiFi...");
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        cpu::delayMs(100);    
        LOG("Scanning networks...");
        int n = WiFi.scanNetworks();
        LOG(n << " networks found:");
        for (int i = 0; i < n; ++i)
            LOG("    " << WiFi.SSID(i) << " @" << WiFi.channel(i) << " (" << WiFi.RSSI(i) << "dbm)");
        WiFi.scanDelete();
    }

private:

    /** \name Web Server
     
        Simple web server that can be used to control the bridge. 
     */
    //@{
    
    //@}

}; // Bridge