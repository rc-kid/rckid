#include <filesystem>
#include <string>

namespace fs = std::filesystem;

inline std::string baseNameOf(fs::path const & path) {
    std::string x{path.filename()};
    size_t baseLength = x.find_first_of("."); 
    return (baseLength == std::string::npos) ? x : x.substr(0, baseLength);
}
