#include <rckid/string.h>

namespace rckid {

    String String::replace(char const * what, char const * with) const {
        uint32_t whatLen = static_cast<uint32_t>(std::strlen(what));
        // nothing to look for, return copy of self
        if (whatLen == 0)
            return *this;
        uint32_t withLen = static_cast<uint32_t>(std::strlen(with));
        char const * src = c_str();
        char const * match = std::strstr(src, what);
        // no occurrence, return copy of self
        if (match == nullptr)
            return *this;
        StringBuilder sb = StringBuilder::withCapacity(size() + 1);
        while (match != nullptr) {
            // copy the part before the match, followed by the replacement
            for (char const * p = src; p != match; ++p)
                sb.appendChar(*p);
            for (uint32_t i = 0; i < withLen; ++i)
                sb.appendChar(with[i]);
            src = match + whatLen;
            match = std::strstr(src, what);
        }
        // copy the remainder after last match
        for (; *src != '\0'; ++src)
            sb.appendChar(*src);
        return sb.str();
    }
}
