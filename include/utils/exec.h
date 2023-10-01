#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <vector>

namespace platform {

    /** Executes given command and returns its standard output. 
     */
    inline std::string execCommand(std::string const & cmd) {
        std::stringstream result;
        FILE * stream = popen(cmd.c_str(), "r");
        if (stream) {
            char buffer[256];
            while (! feof(stream)) {
                size_t bytes = fread(buffer, 1, sizeof(buffer), stream);
                result.write(buffer, bytes);
            }
        }
        return result.str();
    }

} // namespace platform

