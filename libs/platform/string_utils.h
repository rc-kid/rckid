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

inline std::string encodeUTF8(uint32_t codepoint) {
    std::string result;
    if (codepoint <= 0x7F) {
        // 1-byte sequence
        result.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        // 2-byte sequence
        result.push_back(static_cast<char>((codepoint >> 6) | 0xC0));
        result.push_back(static_cast<char>((codepoint & 0x3F) | 0x80));
    } else if (codepoint <= 0xFFFF) {
        // 3-byte sequence
        result.push_back(static_cast<char>((codepoint >> 12) | 0xE0));
        result.push_back(static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80));
        result.push_back(static_cast<char>((codepoint & 0x3F) | 0x80));
    } else if (codepoint <= 0x10FFFF) {
        // 4-byte sequence
        result.push_back(static_cast<char>((codepoint >> 18) | 0xF0));
        result.push_back(static_cast<char>(((codepoint >> 12) & 0x3F) | 0x80));
        result.push_back(static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80));
        result.push_back(static_cast<char>((codepoint & 0x3F) | 0x80));
    } 
    return result;   
}