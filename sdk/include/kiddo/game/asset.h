#pragma once

#include <kiddo/kiddo.h>
#include <kiddo/game/object.h>

namespace kiddo::game {

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

    }; // kiddo::game::Asset

} // namespace kiddo::game
