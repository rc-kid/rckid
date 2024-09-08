#pragma once

/** Wrappers for external libraries that turn compiler warnings in them off. 
 */
#if defined(_MSC_VER)
    #define WARNINGS_OFF() __pragma(warning(push, 0))
    #define WARNINGS_ON()  __pragma(warning(pop))
#else
     #define WARNINGS_OFF \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"") \
        _Pragma("GCC diagnostic ignored \"-Wunused-function\"") \
        _Pragma("GCC diagnostic ignored \"-Wall\"") \
        _Pragma("GCC diagnostic ignored \"-Wextra\"") \
        _Pragma("GCC diagnostic ignored \"-Wpedantic\"")

    #define WARNINGS_ON _Pragma("GCC diagnostic pop")
#endif


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
