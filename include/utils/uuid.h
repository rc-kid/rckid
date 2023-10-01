#pragma once

#if (defined ARCH_LINUX)

#include <uuid/uuid.h>


/** Generates new unique id. 

    To use, the libuuid must be linked with the excutables, i.e. add the following to cmakelists.txt:

    target_link_libraries(YOUR_TARGET uuid) 
 
 */
inline std::string newUuid() {
    uuid_t uid;
    uuid_generate(uid);
    std::stringstream str;
    for (size_t i = 0; i < sizeof(uid); ++i) {
        str << "0123456789abcdef"[uid[i] >> 4];
        str << "0123456789abcdef"[uid[i] & 0xf];
    }
    return str.str();
}



#endif

// TODO add Win32 API calls to the effect