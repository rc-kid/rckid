#include <FatFS/ff.h>
#include <littlefs/lfs.h>

#include <rckid/filesystem.h>

namespace rckid::fs {

    namespace {
        FATFS * fs_ = nullptr;
        lfs_t lfs_;
        lfs_config lfsCfg_;
    } // anonymous namespace

    class FatFSFileReader : public RandomReadStream {
    public:

        ~FatFSFileReader() override {
            f_close(& f_);
        }

        uint32_t read(uint8_t * buffer, uint32_t bufferSize) override {
            UINT bytesRead = 0;
            f_read(& f_, buffer, bufferSize, & bytesRead);
            return bytesRead;
        }

        bool eof() const override {
            return f_eof(& f_);
        }

        uint32_t size() const override {
            return static_cast<uint32_t>(f_size(&f_));
        }

        uint32_t seek(uint32_t position) override {
            f_lseek(& f_, position);
            return static_cast<uint32_t>(f_.fptr);
        }

        uint32_t tell() const override {
            return static_cast<uint32_t>(f_.fptr);
        }

    private:
        FIL f_;
    }; // fs::FatFSFileReader

    class FatFSFileWriter : public rckid::RandomWriteStream {
    public:
        ~FatFSFileWriter() override {
            f_close(& f_);
        }

        uint32_t tryWrite(uint8_t const * buffer, uint32_t numBytes) override {
            UINT bytesWritten = 0; 
            f_write(& f_, buffer, numBytes, & bytesWritten);
            return bytesWritten;
        }

        uint32_t seek(uint32_t position) override {
            f_lseek(& f_, position);
            return static_cast<uint32_t>(f_.fptr);
        }

        uint32_t tell() const override {
            return static_cast<uint32_t>(f_.fptr);
        }

    private:
        FIL f_;
    }; // fs::FatFSFileWriter

    class LittleFSFileReader : public RandomReadStream {
    public:
        ~LittleFSFileReader() override {
            lfs_file_close(& lfs_, & f_);
        }

        uint32_t read(uint8_t * buffer, uint32_t bufferSize) override {
            return lfs_file_read(& lfs_, & f_, buffer, bufferSize);
        }

        bool eof() const override {
            return lfs_file_tell(& lfs_, & f_) == lfs_file_size(& lfs_, & f_);
        }

        uint32_t size() const override {
            return lfs_file_size(& lfs_, & f_);
        }

        uint32_t seek(uint32_t position) override {
            lfs_file_seek(& lfs_, & f_, position, 0);
            return lfs_file_tell(& lfs_, & f_);
        }

        uint32_t tell() const override {
            return lfs_file_tell(& lfs_, & f_);        
        }
        
    private:
        mutable lfs_file_t f_;
    }; // LittleFSFileReader

    class LittleFSFileWriter : public rckid::RandomWriteStream {
    public:
        ~LittleFSFileWriter() override {
            lfs_file_close(& lfs_, & f_);
        }

        uint32_t tryWrite(uint8_t const * buffer, uint32_t numBytes) override {
            return lfs_file_write(& lfs_, & f_, buffer, numBytes);
        }

        uint32_t seek(uint32_t position) override {
            lfs_file_seek(& lfs_, & f_, position, 0);
            return lfs_file_tell(& lfs_, & f_);
        }

        uint32_t tell() const override {
            return lfs_file_tell(& lfs_, & f_);        
        }

    private:
        mutable lfs_file_t f_;

    }; // LittleFSFileWriter


    // path manipulation functions

    String stem(String const & path) {
        if (path.empty())
            return path;
        size_t end = path.size();
        size_t i = end - 1;
        for (; i > 0; --i) {
            if (path[i] == '.')
                end = i;
            else if (path[i] == '/')
                break;
        }
        if (path[i] == '/')
            ++i;
        return path.substr(i, end - i);
    }

    String ext(String const & path) {
        if (path.size() > 1) {    
            for (size_t i = path.size() - 1; i > 0; --i) {
                if (path[i] == '.')
                    return path.substr(i);
            }
        }
        return "";
    }

    String join(String const & path, String const & item) {
        if (path.endsWith('/'))
            return STR(path << item);
        else
            return STR(path << "/" << item);
    }

    String parent(String const & path) {
        for (size_t i = path.size() - 1; i > 0; --i) {
            if (path[i] == '/')
                return path.substr(0, i);
        }
        return "/";
    }

    String root(String const & path) {
        if (path.empty())
            return path;
        uint32_t i = 0;
        if (path[0] == '/')
            ++i; 
        while (i < path.size() && path[i] != '/') 
            ++i;
        return path.substr(0, i);
    }

    // filesystem functions

}