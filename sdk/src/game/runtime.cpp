#include <rckid/game/runtime.h>
#include <rckid/game/object.h>

namespace rckid::game {

    bool Type::operator == (ClassDescriptor const & other) const {
        if (kind_ != Kind::Object)
            return false;
        return descriptor().inheritsFromOrSame(other);
    }

    Type Value::type() const {
        if (kind_ == Type::Kind::Object)
            return Type::Object(& object_->typeDescriptor());
        return Type{kind_, nullptr};
    }

}