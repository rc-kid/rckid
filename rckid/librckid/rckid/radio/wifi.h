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

    /** Initiates a HTTP get message inside given connection. 
     */
    inline void GET(Connection & conn, char const * server) {
    }

} // namespace rckid::radio