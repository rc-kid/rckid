#include <rckid/audio/decoder_stream.h>
#include <rckid/audio/mp3.h>

namespace rckid::audio {

    unique_ptr<DecoderStream> DecoderStream::fromFile(String const & path, fs::Drive drive) {
        auto f = fs::readFile(path, drive); 
        if (f == nullptr)
            return nullptr;
        // TODO maybe some registry of decoders based on file extension or something like that
        if (path.endsWith(".mp3"))
            return std::make_unique<MP3DecoderStream>(std::move(f));
        return nullptr;
    }
    
} // namespace rckid::audio

