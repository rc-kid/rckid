#pragma once

#include <rckid/rckid.h>
#include <rckid/game/descriptors.h>

namespace rckid::game {

    class Engine;

    /** Game object.

        Most objects will render themselves via the render() method, but this is not universally required (such as sound player) and can be disabled via capabilities.    
     */
    class Object {
    public:
        Object() = default;
        Object(String name, [[maybe_unused]] Engine * engine): name_{std::move(name)} {}      

        virtual ~Object() = default;

        String const & name() const { return name_; }

        void setName(String name) {
            name_ = std::move(name);
        }

        /** Renders the game object, if renderable.
         
            Rendering is column based, and is called once per column. Unlike the renderColumn method of ui::Widget, all game objects always get access to the entire column worth of data and must figure out their positioning and visibility on their own.
          */
        virtual void render(Coord column, Color::RGB565 * buffer) {
            ASSERT(typeDescriptor().capabilities().renderable);
        }

        /** Converts current object to a specific subtype. 
         
            Behaves like dynamic_cast, i.e. returns nullptr of the object cannot be converted.
         */
        template<typename T>
        T * as() {
            if (typeDescriptor().inheritsFromOrSame(T::descriptor))
                return static_cast<T*>(this);
            return nullptr;
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
            CAPABILITIES(
                .renderable = true,
            ),
            METHODS(),
            EVENTS()
        );

        /** Returns the game engine object capabilities. 
         */
        virtual ClassDescriptor const & typeDescriptor() const { return descriptor; }

    }; // rckid::game::Object

} // namespace rckid::game
