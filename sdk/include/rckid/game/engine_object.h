#pragma once

#include <rckid/game/event.h>

namespace rckid::game {

    class Engine;

    /** Base class for all dynamically accessible objects in the game engine (assets and game objects). 

        All user accessible stuff in the game comes in the form of game engine objects. Each object must support its class name, and interface declarations. 

        What those have in common is the basic RTTI via knowing its own name and its class name, etc. 
     */
    class EngineObject {
    public:

        EngineObject() = default;

        EngineObject(String name): name_{std::move(name)} { }

        String const & name() const { return name_; }

        void setName(String name) {
            name_ = std::move(name);
        }

        /** Virtual destructor is a must.
         */
        virtual ~EngineObject() = default;

        /** Returns the name of the object class. 
         
            This is used internally by the game engine to keep track of declared actions, properties and events. 
         */
        virtual char const * className() const = 0;

        /** Declares all metadata about the given class. 
         
            Note that metadata is class based, not object based. The game engine makes sure to only call this once per class name.  
         */ 
        virtual void declareInterface(Engine * engine) const {
        }

    private:
        String name_;

    }; // rckid::game::EngineObject

} // namespace rckid::game
