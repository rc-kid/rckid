#pragma once

#include <algorithm> 
#include <cctype>
#include <locale>

inline void ltrimInPlace(std::string & s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

inline void rtrimInPlace(std::string & s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

inline void trimInPlace(std::string & s) {
    rtrimInPlace(s);
    ltrimInPlace(s);
}

inline std::string ltrim(std::string const & s) {
    std::string result{s};
    ltrimInPlace(result);
    return result;
}

inline std::string rtrim(std::string const & s) {
    std::string result{s};
    rtrimInPlace(result);
    return result;
}

inline std::string trim(std::string const & s) {
    std::string result{s};
    trimInPlace(result);
    return result;
}