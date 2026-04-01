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
        
        CLASS_DESCRIPTOR(Asset, assets::icons_24::bookmark,
            "Base class for all assets",
            PARENT(Object),
            CAPABILITIES(
                .renderable = false,
                .constructible = false,
                .passive = true,
            ),
            METHODS(),
            EVENTS()
        );

        ClassDescriptor const & typeDescriptor() const override { return descriptor; }

    }; // rckid::game::Asset

} // namespace rckid::game
