#pragma once

namespace rckid::note {
    #define NOTE(NAME, FREQUENCY, ID) constexpr uint16_t NAME = FREQUENCY;
    #include "notes.inc"
} // namespace rckid::note