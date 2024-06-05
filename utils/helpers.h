#include <filesystem>
#include <string>
#include <sstream>

#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()


namespace fs = std::filesystem;

inline std::string baseNameOf(fs::path const & path) {
    std::string x{path.filename()};
    size_t baseLength = x.find_first_of("."); 
    return (baseLength == std::string::npos) ? x : x.substr(0, baseLength);
}
