#pragma once

#include <cstdint>
#include <functional>


// TODO verify how this works and if it works at all and then change the APIs accordingly to mirrow the writer that changed a lot

class Reader {
public:
    Reader(std::function<uint8_t()> getByte): getByte_{getByte} {}
    
    // TODO override >> for string to type conversion
    
    uint8_t readByte() {
        return getByte_();
    }

    void readBuffer(uint8_t * buffer, size_t size) {
        while (size-- != 0) 
            *(buffer++) = getByte_();
    }

    template<typename T>
    T deserialize(); 

private:
    std::function<uint8_t()> getByte_;

}; // Reader

template<>
inline uint8_t Reader::deserialize<uint8_t>() { 
    return getByte_(); 
}

template<>
inline uint16_t Reader::deserialize<uint16_t>() {
    uint16_t result = getByte_();
    return (getByte_() << 8) | result;
}

template<>
inline std::string Reader::deserialize<std::string>() {
    size_t len = deserialize<uint16_t>();
    std::string result(len, '\0');
    readBuffer(reinterpret_cast<uint8_t*>(result.data()), len);
    return result;
}
