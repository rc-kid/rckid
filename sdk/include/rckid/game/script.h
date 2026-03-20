#pragma once

#include <vector>

#include <rckid/string.h>

namespace rckid::game {

    class Object;

    /** Objects supported by the game script. 
     */
    enum class Type {
        Void,
        Coord,
        Color,
        String,
    }; // rckid::game::Type

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

        // TODO value getters and setters
    private:
        Type type_;
        union {
            Coord coord_;
            Color::RGB565 color_;
            String string_;
        };
    }; // rckid::game::value

    class ArgumentDeclaration {
    public:
    
        ArgumentDeclaration(String name, Type type): name_{std::move(name)}, type_{type} {}

        String const & name() const { return name_; }

        Type type() const { return type_; }

    private:
        String name_;
        Type type_;

    }; // rckid::game::ArgumentDeclaration

    /** Metadata & wrapper around object action. 
     */
    class Action {
    public:

        typedef Value (*Impl)(Object * self, std::initializer_list<Value> args);

        Action(String name, Type returnType, std::vector<ArgumentDeclaration> arguments, Impl impl):
            name_{std::move(name)}, 
            returnType_{returnType}, 
            arguments_{std::move(arguments)}, 
            impl_{std::move(impl)} {
        }

        Value operator () (Object * self,std::initializer_list<Value> args) const {
            return impl_(self, args);
        }

        String const & name() const { return name_; }

        Type returnType() const { return returnType_; }

        std::vector<ArgumentDeclaration> const & arguments() const { return arguments_; }

    private:
        String name_;
        Type returnType_;
        std::vector<ArgumentDeclaration> arguments_;
        Impl impl_;
    }; // rckid::game::Action

} // namespace rckid