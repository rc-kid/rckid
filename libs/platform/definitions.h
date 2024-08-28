#pragma once

#ifndef ASSERT
#define ASSERT(...)
#endif

#ifndef UNREACHABLE
#define UNREACHABLE while (true) {}
#endif

#ifndef UNIMPLEMENTED
#define UNIMPLEMENTED while (true) {}
#endif

#ifndef LOG
#define LOG(...)
#endif

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

/// TODO: This should actually do something
#ifndef  __force_inline
#define  __force_inline
#endif


#if defined(_MSC_VER)
#define NORETURN(...) __declspec(noreturn) __VA_ARGS__
#else
#define NORETURN(...) __VA_ARGS__ __attribute__((noreturn))
#endif
