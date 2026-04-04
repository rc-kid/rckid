#pragma once

#include <rckid/rckid.h>

namespace rckid::game {

    class Engine;
    class Object;
    class ClassDescriptor;

    /** Integer type used everywhere in the game engine. 
     */
    using Integer = Coord;

    class Type {
    public:
        enum class Kind {
            Void, 
            Boolean,
            Integer,
            Point,
            Button,
            Object,
        };

        constexpr static Type Void() { return Type{Kind::Void}; }
        constexpr static Type Boolean() { return Type{Kind::Boolean}; }
        constexpr static Type Point() { return Type{Kind::Point}; }
        constexpr static Type Button() { return Type{Kind::Button}; }
        constexpr static Type Object() { return Type{Kind::Object}; }
        constexpr static Type Object(ClassDescriptor const * descriptor) { return Type{Kind::Object, descriptor}; }

        Kind kind() const { return kind_; }
        ClassDescriptor const & descriptor() const { return *descriptor_; }

        bool operator == (ClassDescriptor const & other) const;

    private:
        friend class Value;

        constexpr Type(Kind kind, ClassDescriptor const * descriptor = nullptr):
            kind_{kind}, descriptor_{descriptor} {
        }

        Kind const kind_;
        ClassDescriptor const * const descriptor_;
    }; 

    class Value {
    public:
        Value(): kind_{Type::Kind::Void}, boolean_{false} {}

        Value(bool value): kind_{Type::Kind::Boolean}, boolean_{value} {}
        Value(Integer value): kind_{Type::Kind::Integer}, integer_{value} {}
        Value(Point value): kind_{Type::Kind::Point}, point_{value} {}
        Value(Btn value): kind_{Type::Kind::Button}, button_{value} {}
        Value(Object * object): kind_{Type::Kind::Object}, object_{object} {}

        Type::Kind kind() const { return kind_; }

        Type type() const;

    private:

        template<typename T>
        friend T as(Value const & v);

        Type::Kind kind_;

        union {
            bool boolean_;
            Integer integer_;
            Point point_;
            Btn button_;
            Object * object_;
        };
    };

} // namespace rckid::game