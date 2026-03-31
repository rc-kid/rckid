#pragma once

#include <rckid/rckid.h>
#include <rckid/game/descriptors.h>

namespace rckid::game {

    class Engine;

    // TODO change Capabilities in App.h to AppCapabilities
    class ObjectCapabilities {
    public:
        /** If true, the game object is renderable and its render method will be called.
         */
        bool renderable = true;
        /** True if the object can be created by user. 
         */
        bool constructible = true;
        /** Passive objects simply hold data and they do not have to be updated every loop iteration. This is typically true for assets.
         */
        bool passive = false;
    }; // rckid::game::ObjectCapabilities

    /** Game object.

        Most objects will render themselves via the render() method, but this is not universally required (such as sound player) and can be disabled via capabilities.    
     */
    class Object {
    public:
        Object() = default;
        Object(String name, [[maybe_unused]] Engine * engine): name_{std::move(name)} {}      

        virtual ~Object() = default;

        /** Returns the game engine object capabilities. 
         */
        virtual ObjectCapabilities capabilities() const {
            return {
                .renderable = true,
            };
        } 

        /** Returns the name of the object class. 
         
            This is used internally by the game engine to keep track of declared actions, properties and events. 
         */
        virtual char const * className() const = 0;

        String const & name() const { return name_; }

        void setName(String name) {
            name_ = std::move(name);
        }

        /** Renders the game object, if renderable.
         
            Rendering is column based, and is called once per column. Unlike the renderColumn method of ui::Widget, all game objects always get access to the entire column worth of data and must figure out their positioning and visibility on their own.
          */
        virtual void render(Coord column, Color::RGB565 * buffer) {
            ASSERT(capabilities().renderable);
        }

    protected:
        friend class Engine;

        virtual void loop() {}

    private:
        String name_;

    public:
        CLASS_DESCRIPTOR(Object, assets::icons_24::bookmark,
            "Base class for all game objects",
            nullptr,
            METHODS(),
            EVENTS()
        );

    }; // rckid::game::Object

} // namespace rckid::game
