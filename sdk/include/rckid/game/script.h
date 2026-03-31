#pragma once

#include <vector>

#include <rckid/string.h>
#include <rckid/game/descriptors.h>

/** Game Engine Script

    What do I need from a typeinfo:

    - class name
    - class icon
    - methods
    - events
    - properties
    
    Ideally this should be constcast and in ROM and each class will define its own static class descriptor. Then a virtual method that returns a pointer to it. That way I we do not pay for the dynamic stuff in RAM, or others (ROM is cheap-ish)
 
 */
namespace rckid::game {

} // namespace rckid