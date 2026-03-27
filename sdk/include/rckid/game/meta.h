#pragma once

#include <rckid/string.h>
#include <rckid/graphics/geometry.h>

namespace rckid::game {

    class Engine;

    /** Integer type used everywhere in the game engine. 
     */
    using Integer = Coord;

    class Object;
    class Object;

    /** Objects supported by the game script. 
     */
    enum class Type {
        Void,
        Integer,
        Point,
        Color,
        String,
    }; // rckid::game::Type

} // namespace rckid::game

namespace rckid::game::meta {

    /** Tagged value for the game script. 
     
        This is a combination of type and the value of that corresponding type.

        Arguably, we do not need the type as even the dynamic parts should be statically-ish type checked, but at the moment this is a lot simpler to implement and we pay for it just the cost of the tag.
     */
    class Value {
    public:
        Value() : type_{Type::Void} {}

        ~Value() {
            if (type_ == Type::String)
                string_.~String();
        }

        Type type() const { return type_; }

        Point point() const {
            ASSERT(type() == Type::Point);
            return point_;
        }

        Point & point() {
            ASSERT(type() == Type::Point);
            return point_;
        }

        Object * object() const {
            // TODO check the type is valid
            return object_;
        }

        Object * & object() {
            // TODO check the type is valid
            return object_;
        }



        // TODO value getters and setters
    private:
        Type type_;
        union {
            Integer coord_;
            Point point_;
            Color::RGB565 color_;
            String string_;
            Object * object_;
        };
    }; // rckid::game::meta::Value

    class ArgumentDeclaration {
    public:
    
        ArgumentDeclaration(String name, Type type): name_{std::move(name)}, type_{type} {}

        String const & name() const { return name_; }

        Type type() const { return type_; }

    private:
        String name_;
        Type type_;

    }; // rckid::game::ArgumentDeclaration

    /** Metadata descriptor and wrapper around a function or method. 
     
        At the descrpitor level, there is no difference between method and a function. Methods take the object itself explicitly as the first argument, while functions do not. 
     */
    class FunctionDescriptor {
    public:

        using Impl = Value (*)(std::initializer_list<Value> args);

        FunctionDescriptor(String name, Type returnType, std::vector<ArgumentDeclaration> arguments, Impl impl):
            name_{std::move(name)}, 
            returnType_{returnType}, 
            arguments_{std::move(arguments)}, 
            impl_{std::move(impl)} {
        }

        Value operator () (std::initializer_list<Value> args) const {
            return impl_(args);
        }

        String const & name() const { return name_; }

        Type returnType() const { return returnType_; }

        std::vector<ArgumentDeclaration> const & arguments() const { return arguments_; }

        // TODO a template method that creates the function descriptor out of a function it gets? 
    private:
        String name_;
        Type returnType_;
        std::vector<ArgumentDeclaration> arguments_;
        Impl impl_;
    }; // rckid::game::meta::FunctionDescriptor

    /** Game object property descriptor. 
     
        Properties have name and type. A property is expected to have getter, and can also have a setter. 

        TODO TODO 
     */
    class PropertyDescriptor {

    }; // rckid::game::meta::PropertyDescriptor

    /** Descriptor for an event. 
     
        Unlike functions, all events must belong to some class. The event descriptor must be able to append handlers as well as fire.
     */
    class EventDescriptor {
    public:
        String const & name() const { return name_; }

    private:

        // TODO impl functions for clearing, emiting and appending to the event. Generated during registration

        String name_;



    }; // rckid::game::meta::EventDescriptor

    class ClassDescriptor {
    public:

        typedef Object * (*Constructor)(String name, Engine * engine);

        /** Creates instance of the given class.
         */
        Object * create(String name, Engine * engine) { return constructor_(std::move(name), engine); } 

        FunctionDescriptor * getAction(String const & name) const {
            for (auto & action : actions_)
                if (action->name() == name)
                    return action.get();
            return nullptr;
        }

        EventDescriptor * getEvent(String const & name) const {
            for (auto & event : events_)
                if (event->name() == name)
                    return event.get();
            return nullptr;
        }

        // TODO ideally, registration should be as simple as this
        template<typename RESULT, typename... ARGS>
        FunctionDescriptor * registerFunction(String name, RESULT (* f)(ARGS...)) {
            UNIMPLEMENTED;
        }

    private:
        String name_;
        
        Constructor constructor_;
        
        std::vector<unique_ptr<FunctionDescriptor>> actions_;

        std::vector<unique_ptr<EventDescriptor>> events_;

    }; // rckid::game::meta::ClassDescriptor




} // namespace rckid::game::meta