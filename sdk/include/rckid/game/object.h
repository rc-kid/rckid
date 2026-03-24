#pragma once

#include <rckid/rckid.h>
#include <rckid/game/engine_object.h>

namespace rckid::game {

    // TODO change Capabilities in App.h to AppCapabilities
    class ObjectCapabilities {
    public:
        /** If true, the game object is renderable and its render method will be called.
         */
        bool renderable = true;
    }; // rckid::game::ObjectCapabilities

    /** Game object.

        Most objects will render themselves via the render() method, but this is not universally required (such as sound player) and can be disabled via capabilities.    
     */
    class Object : public EngineObject {
    public:
        Object() = default;
        Object(String name): EngineObject{std::move(name)} {}      

        /** Returns the game engine object capabilities. 
         */
        virtual ObjectCapabilities capabilities() const {
            return {
                .renderable = true,
            };
        } 

        /** Renders the game object, if renderable.
         
            Rendering is column based, and is called once per column. Unlike the renderColumn method of ui::Widget, all game objects always get access to the entire column worth of data and must figure out their positioning and visibility on their own.
          */
        virtual void render(Coord column, Color::RGB565 * buffer) {
            ASSERT(capabilities().renderable);
        }

    }; // rckid::game::Object

} // namespace rckid::game
