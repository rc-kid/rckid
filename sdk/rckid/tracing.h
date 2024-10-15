#pragma once

/** \defgroup tracing Tracing

    To aid with debugging, rckid supports a rather simple tracing mechanism via the TRACE macro, which is identical to the LOG macro for serial output, but a separate macro is used in case different behavior is required in the future. 

    Apart from the generic TRACE macro, the SDK defines multile tracing levels that each have their own macro and can be enabled or disabled by commandline to enable selective tracing of only certain trace levels. To enable the tracing, simply add the tracing name macro as a definition via cmake, e.g. -DTRACE_MEMORY, etc. 

    The followingtrace levels are supported:

    - TRACE_MEMORY for memory related events (entering & leaving arenas, etc)
    - TRACE_MENU_APP for tracing the main menu application behavior

 */

#define TRACE(...) do { LOG(__VA_ARGS__); } while (false)

//#define TRACE_MENU_APP
//#define TRACE_MEMORY

#if !defined TRACE_MENU_APP
    #define TRACE_MENU_APP(...)
#else
    #undef TRACE_MENU_APP
    #define TRACE_MENU_APP(...) TRACE("menuApp:" << __VA_ARGS__)
#endif

#if !defined TRACE_MEMORY
    #define TRACE_MEMORY(...)
#else
    #undef TRACE_MEMORY
    #define TRACE_MEMORY(...) TRACE("memory:" << __VA_ARGS__)
#endif


#if !defined TRACE_TONE
    #define TRACE_TONE(...)
#else
    #undef TRACE_TONE
    #define TRACE_TONE(...) TRACE("memory:" << __VA_ARGS__)
#endif

