#pragma once

#include <rckid/game/descriptors.h>
#include <rckid/game/object.h>

namespace rckid::game {

    /** Converts current object to a specific subtype. 
         
        Behaves like dynamic_cast, i.e. returns nullptr of the object cannot be converted.
    */
    template<typename T>
    inline T * as(Object * obj) {
        if (obj->typeDescriptor().inheritsFromOrSame(T::descriptor))
            return static_cast<T*>(obj);
        return nullptr;
    }

    template<typename T>
    T as(Value const & v) {
        ASSERT(v.kind_ == Type::Kind::Object);
        return as<T>(v.object_);
    }

    template<>
    inline Object * as<Object *>(Value const & v) {
        ASSERT(v.kind_ == Type::Kind::Object);
        return v.object_;
    }

    template<>
    inline bool as<bool>(Value const & v) {
        ASSERT(v.kind_ == Type::Kind::Boolean);
        return v.boolean_;
    }

    template<>
    inline Integer as<Integer>(Value const & v) {
        ASSERT(v.kind_ == Type::Kind::Integer);
        return v.integer_;
    }

    template<>
    inline Point as<Point>(Value const & v) {
        ASSERT(v.kind_ == Type::Kind::Point);
        return v.point_;
    }

    template<>
    inline Btn as<Btn>(Value const & v) {
        ASSERT(v.kind_ == Type::Kind::Button);
        return v.button_;
    }

} // namespace rckid::game