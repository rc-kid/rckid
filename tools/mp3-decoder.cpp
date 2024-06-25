#include <iostream>
#include <fstream>
#include <vector>

#include "../libs/libhelix-mp3/mp3dec.h"



int main(int argc, char * argv[]) {
    HMP3Decoder mp3 = MP3InitDecoder();
    int16_t buffer[4096];
    std::ifstream input(argv[1], std::ios::binary);
    std::vector<char> bytes(
         (std::istreambuf_iterator<char>(input)),
         (std::istreambuf_iterator<char>()));
    input.close();   
    std::cout << "Loaded " << bytes.size() << "bytes..." << std::endl;
    size_t i = 0;
    unsigned char * bufIn = reinterpret_cast<unsigned char*>(bytes.data());
    //while ( i < input.size()) {
        int bytesLeft = bytes.size() - i;
        if (bytesLeft > 2048)
            bytesLeft = 2048;
        int res = MP3Decode(mp3, & bufIn,  & bytesLeft, buffer, 0);
        std::cout << "Result : " << res << std::endl;
        std::cout << "Bytes left: " << bytesLeft << std::endl;
        MP3FrameInfo fInfo;
        MP3GetLastFrameInfo(mp3, & fInfo);
        std::cout << "Bitrate:         " << fInfo.bitrate << std::endl;
        std::cout << "Channels:        " << fInfo.nChans << std::endl;
        std::cout << "Sample Rate:     " << fInfo.samprate << std::endl;
        std::cout << "Bits per sample: " << fInfo.bitsPerSample << std::endl;
        std::cout << "Output samples:  " << fInfo.outputSamps << std::endl;
        std::cout << "Layer:           " << fInfo.layer << std::endl;
        std::cout << "Version:         " << fInfo.version << std::endl;
    //}    


    MP3FreeDecoder(mp3);
}