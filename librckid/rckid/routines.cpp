#include "rckid.h"



extern "C" {

    void memFill32(uint32_t * buffer, uint32_t size, uint32_t value) __attribute__((weak));
    void rckid_mem_fill_32x8(uint32_t * target, size_t num, uint32_t source)  __attribute__((weak)); 
    uint8_t const * rckid_color256_to_rgb(uint8_t const * in, uint16_t * out, unsigned numPixels, uint16_t const * palette)  __attribute__((weak));


    void memFill32(uint32_t * buffer, uint32_t size, uint32_t value) {
        while (size-- > 0)
            *(buffer++) = value;
    }

    void rckid_mem_fill_32x8(uint32_t * buffer, size_t num, uint32_t value) {
        while (num-- > 0)
            *(buffer++) = value;
    }

    uint8_t const * rckid_color256_to_rgb(uint8_t const * in, uint16_t * out, unsigned numPixels, uint16_t const * palette) {
        while (numPixels-- > 0)
            *(out++) = palette[*in++];
        return in;
    }
    
}
