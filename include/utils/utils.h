#pragma once

#include <cassert>
#include <cstring>
#include <vector>

#include <sstream>

#if (defined _WIN32)
    #define ARCH_WINDOWS
#elif (defined __linux__)
    #define ARCH_LINUX
#else
    #error "Unsupported utils platform"
#endif

#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()

#define UNREACHABLE throw std::runtime_error{STR("Unreachable code triggered at " << __FILE__ << ":" << __LINE__)};
#define UNIMPLEMENTED throw std::runtime_error{STR("Unimplemented code triggered at " << __FILE__ << ":" << __LINE__)};

#define ASSERT(...) assert(__VA_ARGS__)

namespace str {

    inline bool isWhitespace(char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    inline void trimLeft(std::string &s) {
        size_t i = 0;
        while (i < s.size() && isWhitespace(s[i]))
            ++i;
        s.erase(s.begin(), s.begin() + i);
    }

    inline void trimRight(std::string &s) {
        size_t i = s.size();
        while (i > 0 && isWhitespace(s[i -1]))
            --i;
        s.erase(s.begin() + i, s.end());
    }

    inline void trim(std::string & s) {
        trimLeft(s);
        trimRight(s);
    }

    inline std::string escape(std::string const & str) {
        std::stringstream s;
        for (char c : str) {
            switch (c) {
                case '"':
                case '\'':
                case '\\':
                    s << '\\' << c;
                    break;
                case '\n':
                    s << "\\n";
                    break;
                case '\t':
                    s << "\\t";
                    break;
                case '\r':
                    s << "\\r";
                    break;
                case '\0':
                    s << "\\0";
                    break;
                default:
                    s << c;
                    break;
            }
        }
        return s.str();
    }

    /** Returns true if given string ends with the given suffix. 
     
        Does not use the evil string object. 
    */
    inline bool endsWith(char const * str, char const * suffix) {
        unsigned l1 = strlen(str);
        unsigned l2 = strlen(suffix);
        return strncmp(str + (l1 - l2), suffix, l2) == 0;
    }

    inline bool startsWith(std::string const & s, char const * prefix) {
        return (s.rfind(prefix, 0) == 0);
    }

    inline std::vector<std::string> vector(std::initializer_list<std::string_view> items) {
        std::vector<std::string> result;
        for (auto s : items)
            result.push_back(std::string{s});
        return result;
    }

} // namespace str

/** A simple hasher template that delegates the hashing to the object being hashed. It's simpler and just saves the need to reimplement the std::hash for the object. 

 */
template<typename T>
struct Hasher {
    size_t operator() (T const & x) const {
        return x.hash();
    }
}; 


template<typename T>
bool setIfDiffers(T & variable, T const & value) {
    if (variable == value)
        return false;
    variable = value;
    return true;
}