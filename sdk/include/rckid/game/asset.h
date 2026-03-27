#pragma once

#include <rckid/rckid.h>
#include <rckid/game/object.h>

namespace rckid::game {

    /** Base class for all game assets. 
     
        
     */
    class Asset : public Object {
    public:
        // TODO serialization and deserialization by virtual table

        Asset() = default;
        Asset(String name, Engine * engine): Object{std::move(name), engine} {}     
        
        ObjectCapabilities capabilities() const override {
            return {
                .renderable = false,
                .constructible = true, 
                .passive = true
            };
        }

    }; // rckid::game::Asset

} // namespace rckid::game
