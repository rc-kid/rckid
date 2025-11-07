#include "../filesystem.h"
#include "mp3.h"
#include "audio.h"

namespace rckid {

    AudioStream * AudioStream::fromFile(String const & path) {
        auto f = std::make_unique<fs::FileRead>(fs::fileRead(path));
        if (!f->good())
            return nullptr;
        String ext = fs::ext(path);
        if (ext == ".mp3")
            return new MP3Stream{std::move(f), fs::stem(path)};
        else
            return nullptr;
    }

} // namespace rckid