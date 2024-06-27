#pragma once

namespace rckid::radio {

    /** Defines a connection between two devices, or a device and base station. 
     
        Once opened, the connection can be written to and read from to move data to/from the other device. When the connection is created, the HW layer contacts the targer device and asks for a new connection to be created with itself. 
     */
    class Connection {

    };

} // namespace rckid::radio