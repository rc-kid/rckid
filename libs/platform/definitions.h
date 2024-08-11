#pragma once


/** Because Microsoft and GCC/CLang have different standards of how to declare packed structs or classes. Just wrap the definition with the PACKED() macro. 
 */
#if defined(_MSC_VER)
#define PACKED(...) \
    __pragma(pack(push, 1)) \
    __VA_ARGS__ ; \
    __pragma(pack(pop)) \
    static_assert(true)
#else
#define PACKED(...) __VA_ARGS__ __attribute__((packed))  
#endif