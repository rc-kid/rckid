#pragma once

#include "stream.h"

namespace rckid::ini {

    /** Parser for simplified INI  */
    class Parser {
    public:
        Parser(ReadStream & from):
            from_{from} {
        }

    private:
        ReadStream & from_;

    }; // rckid::ini::Reader


} // namespace rckid::ini