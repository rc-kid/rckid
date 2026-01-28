#include <fstream>
#include <filesystem>

#include <rckid/filesystem.h>

#include "system_malloc_guard.h"

namespace rckid::fs {


    using namespace rckid::internal::memory;

    bool sdMounted_ = false;
    bool cartridgeMounted_ = false;
    std::filesystem::path sdRoot_;
    std::filesystem::path cartridgeRoot_;

    std::filesystem::path getHostPath(Drive dr, String const & path) {
        std::filesystem::path result{path.c_str()[0] == '/' ? path.c_str() + 1 : path.c_str()};
        return (dr == Drive::SD) ? sdRoot_ / result : cartridgeRoot_ / result;
    }

    class FileReader : public RandomReadStream {
    public:
        FileReader(std::ifstream && f) : f_{std::move(f)} {
            fileLength_ = size();
        }

        uint32_t read(uint8_t * buffer, uint32_t bufferSize) override {
            f_.read(reinterpret_cast<char *>(buffer), bufferSize);
            return static_cast<uint32_t>(f_.gcount());
        }

        bool eof() const override {
            return f_.tellg() == fileLength_;
        }

        uint32_t size() const override {
            std::streampos currentPos = f_.tellg();
            f_.seekg(0, std::ios::end);
            std::streamsize size = f_.tellg();
            f_.seekg(currentPos);
            return static_cast<uint32_t>(size);
        }

        uint32_t seek(uint32_t position) override {
            f_.seekg(position, std::ios::beg);
            std::streamsize size = f_.tellg();
            return static_cast<uint32_t>(size);
        }

        uint32_t tell() const override {
            std::streamsize size = f_.tellg();
            return static_cast<uint32_t>(size);
        }

    private:

        mutable std::ifstream f_;
        uint32_t fileLength_;

    }; 

    class FileWriter : public RandomWriteStream {
    public:
        FileWriter(std::ofstream && f) : f_{std::move(f)} {
        }

        uint32_t tryWrite(uint8_t const * buffer, uint32_t numBytes) override {
            f_.write(reinterpret_cast<char const *>(buffer), numBytes);
            return numBytes;
        }

        uint32_t seek(uint32_t position) override {
            f_.seekp(position, std::ios::beg);
            std::streamsize size = f_.tellp();
            return static_cast<uint32_t>(size);
        }

        uint32_t tell() const override {
            std::streamsize size = f_.tellp();
            return static_cast<uint32_t>(size);
        }

    public:

        mutable std::ofstream f_;

    };

#ifdef RCKID_CUSTOM_FILESYSTEM

    /** Initializes the virtual filesystem. 
     
        This is a very simple procedure - takes the current working dir and looks for cartridge and sd folder there. If found, those are used as cartidge and sd roots. 
     */
    void initializeFilesystem() {
        LOG(LL_INFO, "Initializing virtual filesystem...");
        SystemMallocGuard g_;
        std::filesystem::path currentPath = std::filesystem::current_path();
        LOG(LL_INFO, "    cwd: " << currentPath.c_str());
        std::filesystem::path sdPath = currentPath / "sd";
        if (std::filesystem::exists(sdPath) && std::filesystem::is_directory(sdPath)) {
            sdRoot_ = sdPath;
            sdMounted_ = true;
            LOG(LL_INFO, "    SD root: " << sdRoot_.c_str());
        }
        std::filesystem::path cartPath = currentPath / "cartridge";
        if (std::filesystem::exists(cartPath) && std::filesystem::is_directory(cartPath)) {
            cartridgeRoot_ = cartPath;
            cartridgeMounted_ = true;
            LOG(LL_INFO, "    Cartridge root: " << cartridgeRoot_.c_str());
        }
    }

    bool isMounted(Drive dr) {
        switch (dr) {
            case Drive::SD:
                return sdMounted_;
            case Drive::Cartridge:
                return cartridgeMounted_;
        }
        UNREACHABLE;
    }

    bool format(Drive dr) {
        try {
            SystemMallocGuard g_;
            switch (dr) {
                case Drive::SD:
                    if (sdRoot_.empty())
                        return false;
                    std::filesystem::remove_all(sdRoot_.c_str());
                    break;
                case Drive::Cartridge:
                    if (cartridgeRoot_.empty())
                        return false;
                    std::filesystem::remove_all(cartridgeRoot_.c_str());
                    break;
                default:
                    return false;
            }
            return true;
        } catch (...) {
            return false;
        }
    } 

    bool mount(Drive dr) {
        switch (dr) {
            case Drive::SD:
                if (sdRoot_.empty())
                    return false;
                sdMounted_ = true;
                return true;
            case Drive::Cartridge:
                if (cartridgeRoot_.empty())
                    return false;
                cartridgeMounted_ = true;
                return true;
        }
        UNREACHABLE;
    }

    void unmount(Drive dr) {
        switch (dr) {
            case Drive::SD:
                sdMounted_ = false;
                break;
            case Drive::Cartridge:
                cartridgeMounted_ = false;
                break;
        }
        UNREACHABLE;
    }

    uint64_t getCapacity(Drive dr) {
        if (!isMounted(dr))
            return 0;
        try {
            SystemMallocGuard g_;
            switch (dr) {
                case Drive::SD:
                    if (sdRoot_.empty())
                        return 0;
                    return std::filesystem::space(sdRoot_.c_str()).capacity;
                case Drive::Cartridge:
                    if (cartridgeRoot_.empty())
                        return 0;
                    return std::filesystem::space(cartridgeRoot_.c_str()).capacity;
                default:
                    return 0;
            }
        } catch (...) {
            return 0;
        }
    }

    uint64_t getFreeCapacity(Drive dr) {
        if (!isMounted(dr))
            return 0;
        try {
            SystemMallocGuard g_;
            switch (dr) {
                case Drive::SD:
                    if (sdRoot_.empty())
                        return 0;
                    return std::filesystem::space(sdRoot_.c_str()).available;
                case Drive::Cartridge:
                    if (cartridgeRoot_.empty())
                        return 0;
                    return std::filesystem::space(cartridgeRoot_.c_str()).available;
                default:
                    return 0;
            }
        } catch (...) {
            return 0;
        }
    }

    Filesystem getFormat(Drive dr) {
        if (!isMounted(dr))
            return Filesystem::Unrecognized;
        switch (dr) {
            case Drive::SD:
                if (! sdRoot_.empty())
                    return Filesystem::exFAT;
                break;
            case Drive::Cartridge:
                if (! cartridgeRoot_.empty())
                    return Filesystem::LittleFS;
                break;
            default:
                break;
        }
        return Filesystem::Unrecognized;
    }

    String getLabel(Drive dr) {
        switch (dr) {
            case Drive::SD: {
                return sdRoot_.c_str();
            }
            case Drive::Cartridge:
                return cartridgeRoot_.c_str();
        }
        UNREACHABLE;
    }

    bool exists(String const & path, Drive dr) {
        if (!isMounted(dr))
            return false;
        SystemMallocGuard g;
        std::filesystem::path p{getHostPath(dr, path)};
        return std::filesystem::exists(p);
    }

    bool isFolder(String const & path, Drive dr) {
        if (!isMounted(dr))
            return false;
        SystemMallocGuard g;
        std::filesystem::path p{getHostPath(dr, path)};
        return std::filesystem::is_directory(p);
    }

    bool isFile(String const & path, Drive dr) {
        SystemMallocGuard g;
        std::filesystem::path p{getHostPath(dr, path)};
        return std::filesystem::is_regular_file(p);
    }

    bool createFolder(String const & path, Drive dr) {
        if (!isMounted(dr))
            return false;
        SystemMallocGuard g_;
        std::filesystem::path p{getHostPath(dr, path)};
        return std::filesystem::create_directory(p);
    }

    bool createFolders(String const & path, Drive dr) {
        if (!isMounted(dr))
            return false;
        if (path == nullptr || path[0] == 0)
            return false;
        uint32_t i = 0;
        if (path[0] == '/') 
            ++i;
        while (true) {
            if (path[i] == '/' || path[i] == 0) {
                String p = path.substr(i);
                if (! isFolder(p.c_str(), dr) &&  (! createFolder(p.c_str(), dr)))
                    return false;
                if (path[i] == 0)
                    break;
            }
            ++i;
        }
        return true;
    }

    bool eraseFile(String const & path, Drive dr) {
        if (!isMounted(dr))
            return false;
        SystemMallocGuard g;
        std::filesystem::path p{getHostPath(dr, path)};
        return std::filesystem::remove(p);
    }

    unique_ptr<RandomReadStream> readFile(String const & path, Drive dr) {
        if (! isMounted(dr))
            return nullptr;
        SystemMallocGuard g;
        std::filesystem::path p{getHostPath(dr, path)};
        std::ifstream f{p, std::ios::binary};
        if (!f.is_open())
            return nullptr;
        return unique_ptr<RandomReadStream>{new FileReader{std::move(f)}};
    }

    unique_ptr<RandomWriteStream> writeFile(String const & path, Drive dr) {
        if (! isMounted(dr))
            return nullptr;
        SystemMallocGuard g;
        std::filesystem::path p{getHostPath(dr, path)};
        std::ofstream f{p,  std::ios::binary | std::ios::trunc};
        if (!f.is_open())
            return nullptr;
        return unique_ptr<RandomWriteStream>{new FileWriter{std::move(f)}};
    }

    unique_ptr<RandomWriteStream> appendFile(String const & path, Drive dr) {
        if (! isMounted(dr))
            return nullptr;
        SystemMallocGuard g;
        std::filesystem::path p{getHostPath(dr, path)};
        std::ofstream f{p,  std::ios::binary | std::ios::app};
        if (!f.is_open())
            return nullptr;
        return unique_ptr<RandomWriteStream>{new FileWriter{std::move(f)}};
    }

    uint32_t readFolder(String const & path, Drive dr, std::function<void(FolderEntry const &)> callback) {
        if (! isMounted(dr))
            return 0;
        SystemMallocGuard g;
        std::filesystem::path p{getHostPath(dr, path)};
        uint32_t count = 0;
        try {
            for (const auto & entry : std::filesystem::directory_iterator{p}) {
                g.release();
                FolderEntry fe;
                fe.name = entry.path().filename().c_str();
                fe.isFolder = entry.is_directory();
                fe.size = entry.is_regular_file() ? static_cast<uint32_t>(entry.file_size()) : 0;
                callback(fe);
                ++count;
                g.acquire();
            }
        } catch (...) {
            LOG(LL_ERROR, "Failed to read folder: " << path);
        }
        return count;
    }
#endif // RCKID_CUSTOM_FILESYSTEM

} // namespace rckid::fs