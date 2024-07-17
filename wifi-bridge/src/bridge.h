#pragma once

#include <platform.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include "rckid/radio/radio.h"
#include "rckid/radio/wifi.h"

/** Connection
 
    Connect with server and port, then any connection input data, send to the http client, any received data, send to the connection buffer

    when closed, close the connection, when connection is closed externally, close the connection here
 
    The bridge is a controller really. 
    The wifi is then just WiFi control stuff. 

 */



/** The RCKid HTTP Bridge
 
    The bridge is the core part of both the base station and the WiFi cartridge. It provides a simple API for sending HTTP Requests and getting HTTP responses. The bridge is agnostic about the HTTP requests location, the decoding of which is left to the RCKid communicating with it. 
        
    The bridge itself is configurable via the same HTTP interface thanks to a simple web server it includes that utilizes AJAX requests to set various bridge features and a minimal human readable web interface built on top of those so that it can be configured from a browser. 

    From a security perspective, the bridge is extremely primitive. Security of the web server and configuration via it is provided by the WiFi network's security the bridge connects to (i.e. anyone who has access to the network has access to the bridge), while the security of the communication between RCKid itself and the bridge is delegated to the layer used. 
 */
class Bridge : public rckid::radio::Controller {
public:
protected:

    /** Allows only one simultaneous connection. 
     
        If accepted, depending on the connection type, we attach metadata to be either http or https client
     */
    void onConnectionRequest(rckid::radio::msg::ConnectionOpen const & request) override {
        if (numConnections() == 0)
            acceptConnection(request);
        else
            rejectConnection(request);
    }

    /** Called when the client closes the connection. 
     
        Close the underlying 
     */
    void onConnectionClosed(rckid::radio::Connection & conn, char const * etxra) override {
        using namespace rckid::radio::wifi;
        switch (conn.param()) {
            case CMD_HTTP_GET:
                http_.stop();
                break;
            case CMD_HTTPS_GET:
                https_.stop();
                break;
            default:
                break;
        }
    }

    /** When connection has data, we can:
     
        - figure out what type of connection we are dealing with (first byte)
     */
    void onConnectionDataReady(rckid::radio::Connection & conn) override {
        using namespace rckid::radio::wifi;
        switch (conn.param()) {
            case CMD_CONNECT:
                // TODO 
                UNIMPLEMENTED;
                break;
            case CMD_DISCONNECT:
                wifi::disconnect();
                closeConnection(conn);
                break;
            case CMD_HTTP_GET:
                // if the connection is already opened, simply transfer the bytes
                if (conn.hasMetadata()) {
                    while (true) {
                        unsigned cb = conn.canReadContinuous();
                        if (cb == 0)
                            break;
                        http_.write(conn.readBuffer(), cb);
                    }
                // otherwise see if there is enough for the server address and connect
                } else if (conn.canRead<std::string>()) {
                    std::string serverName{conn.reader().deserialize<std::string>()};
                    http_.connect(serverName.c_str(), 80);
                    conn.setMetadata(& http_);
                }
                break;
            case CMD_HTTPS_GET:
                // if the connection is already opened, simply transfer the bytes
                if (conn.hasMetadata()) {
                    while (true) {
                        unsigned cb = conn.canReadContinuous();
                        if (cb == 0)
                            break;
                        https_.write(conn.readBuffer(), cb);
                    }
                // otherwise see if there is enough for the server address and connect
                } else if (conn.canRead<std::string>()) {
                    std::string serverName{conn.reader().deserialize<std::string>()};
                    https_.connect(serverName.c_str(), 443);
                    conn.setMetadata(& https_);
                } 
                break;
            default:
                UNREACHABLE;
                break;
        }
    }

private:

    static inline WiFiClient http_;
    static inline WiFiClientSecure https_;  

}; // Bridge