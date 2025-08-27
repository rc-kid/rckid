#pragma once

#ifdef __cplusplus
namespace platform {
    /** Endiannes of the platform. s
     */
    enum class Endian {
        Little, 
        Big, 
    }; 

} // namespace platform
#endif


/** Because Microsoft and GCC/CLang have different standards of how to declare packed structs or classes. Just wrap the definition with the PACKED() macro. 
 */
#if defined(_MSC_VER)
#define PACKED(...) \
    __pragma(pack(push, 1)) \
    __VA_ARGS__ ; \
    __pragma(pack(pop)) \
    static_assert(true)
// TODO this is not right
#define PACKED_ALIGNED(N, ...) PACKED(__VA_ARGS__)
#else
#define PACKED(...) __VA_ARGS__ __attribute__((packed))
#define PACKED_ALIGNED(N, ...) __VA_ARGS__ __attribute__((packed, aligned(N)))
#endif

#ifdef _MSC_VER
    #define FORCE_INLINE(...) __forceinline __VA_ARGS__
#elif defined(__GNUC__) || defined(__clang__)
    #define FORCE_INLINE(...) __attribute__((always_inline)) inline __VA_ARGS__
#else
    #define FORCE_INLINE(...) inline __VA_ARGS__
#endif

#if defined(_MSC_VER)
#define NORETURN(...) __declspec(noreturn) __VA_ARGS__
#else
#define NORETURN(...) __attribute__((noreturn)) __VA_ARGS__
#endif




#ifdef FOOBAR

/** Wrappers for external libraries that turn compiler warnings in them off. 
 */
#if defined(_MSC_VER)
    #define WARNINGS_OFF __pragma(warning(push, 0))
    #define WARNINGS_ON  __pragma(warning(pop))
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

#endif
