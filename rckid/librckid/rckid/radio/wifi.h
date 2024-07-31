#pragma once

#include "radio.h"

/** \name Internet Connections
 
    Provides an internet support layer over the radio connections. 

    HTTP_GET

    HTTPS_GET
    
 */
namespace rckid::radio::wifi {

    constexpr uint8_t CMD_CONNECT = 1;
    constexpr uint8_t CMD_DISCONNECT = 2;
    constexpr uint8_t CMD_HTTP_GET = 3;
    constexpr uint8_t CMD_HTTPS_GET = 4;

    class WiFiController : public Controller {
    protected:

        /** Opens a HTTP GET connection for given URL. 
         */
        Connection * HTTP_GET(char const * serverName, std::string const & path) {
            Connection * conn = openConnection(internetDevice_, CMD_HTTP_GET);
            // write the server name first, followed by the GET request with given path (and parameters. As per the HTTP standard, this has to be followed by two newlines
            conn->writer() << serialize(serverName) << "GET " << path << " HTTP/1.1\r\n\r\n";
            return conn;
        }

    private:

        DeviceId internetDevice_; 

    }; // WiFiController

} // namespace rckid::radio