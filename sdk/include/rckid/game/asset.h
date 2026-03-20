#pragma once

#include <rckid/rckid.h>
#include <rckid/game/engine_object.h>

namespace rckid::game {

    /** Base class for all game assets. 
     
        
     */
    class Asset : public EngineObject {
    public:
        // TODO serialization and deserialization by virtual table

    }; // rckid::game::Asset

} // namespace rckid::game