#pragma once

#include "../rckid.h"

/** Logging 
 
    RCKid SDK provides very simple, but extensible logging mechanism. Each log level is a macro that must evaluate to '1' if the log level is to be enabled, '0' or not defined otherwise. This way log levels can be enable statically during compilation by adding extra defines to the compiler, e.g:

        cmake .. -DLL_DEBUG=1

    The LOG macro takes two arguments - log level and expression to be logged, which is passed to a Writer obtained by the platform-specific debugWrite() function (this is stdout for the fantasy console, and USB/UART for mkIII). As the loglevel check is static, there is no runtime cost associated with disable log levels as the code is optimized away, but even disabled log levels must still compile. 

    The SDK defines some basic log levels with common meanings, see below in this file for their quick description.
 */

#define IS_LOGLEVEL_ENABLED_HELPER(X) #X
#define IS_LOGLEVEL_ENABLED(X) (IS_LOGLEVEL_ENABLED_HELPER(X)[0] == '1')
#define LOG(LOGLEVEL,...) do { if (IS_LOGLEVEL_ENABLED(LOGLEVEL)) rckid::debugWrite() << #LOGLEVEL << ": " << __VA_ARGS__ << '\n'; } while (false)

/** Log level for errors.  
 
    On by default, used for recoverable errors, such as invalid commands, failed transactions, or wrong data formats that the SDK will just ignore and continue.
*/
#ifndef LL_ERROR
#define LL_ERROR 1
#endif

/** Log level for warnings. 
 
    Alerts to unexpected situations that should *not* happen, but which do not interfere with the device functionality (i.e. their ignoring can be complete). On by default. 
 */
#ifndef LL_WARN
#define LL_WARN 1
#endif

/** Log level for informational messages.
 
    The SDK uses those graciously to inform about various state changes, etc. It is on by default in non-release builds, otherwise off.
 */
#ifndef LL_INFO
  #ifdef NDEBUG
    #define LL_INFO 0
  #else
    #define LL_INFO 1
  #endif
#endif

/** Debugging log level.
 
    Messages under this log level are only useful when debugging certain very specific features and as such are disabled by default.
 */
#ifndef LL_DEBUG
#define LL_DEBUG 0
#endif
