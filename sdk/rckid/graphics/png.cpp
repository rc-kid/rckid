#include "png.h"

// forward references
PNG_STATIC int PNGInit(PNGIMAGE *pPNG);
PNG_STATIC int DecodePNG(PNGIMAGE *pImage, void *pUser, int iOptions);
PNG_STATIC uint8_t PNGMakeMask(PNGDRAW *pDraw, uint8_t *pMask, uint8_t ucThreshold);
// Include the C code which does the actual work
#include <PNGdec/src/png.inl>

int32_t readStream(PNGFILE * pFile, uint8_t * pBuf, int32_t iLen) {
    rckid::RandomReadStream * s = static_cast<rckid::RandomReadStream*>(pFile->fHandle);
    return s->read(pBuf, iLen);
}

int32_t seekStream(PNGFILE * pFile, int32_t iPosition) {
    rckid::RandomReadStream * s = static_cast<rckid::RandomReadStream*>(pFile->fHandle);
    return s->seek(iPosition);
}

typedef int32_t (PNG_SEEK_CALLBACK)(PNGFILE *pFile, int32_t iPosition);


namespace rckid {

    PNG PNG::fromStream(RandomReadStream & stream) {
        PNG result;
        result.ucMemType = PNG_MEM_RAM;
        result.pfnRead = readStream;
        result.pfnSeek = seekStream;
        result.pfnDraw = decodeLine_;
        result.pfnOpen = nullptr;
        result.pfnClose = nullptr;
        result.PNGFile.iSize = stream.size();
        //result.PNGFile.pData = const_cast<uint8_t *>(buffer);
        result.PNGFile.fHandle = & stream;
        PNGInit(& result);
        return result;
    }

    PNG PNG::fromBuffer(uint8_t const * buffer, uint32_t numBytes) {
        PNG result;
        result.ucMemType = PNG_MEM_RAM;
        result.pfnRead = readRAM;
        result.pfnSeek = seekMem;
        result.pfnDraw = decodeLine_;
        result.pfnOpen = nullptr;
        result.pfnClose = nullptr;
        result.PNGFile.iSize = numBytes;
        result.PNGFile.pData = const_cast<uint8_t *>(buffer);
        PNGInit(& result);
        return result;
    }

    int PNG::decode(DecodeCallback cb) {
        cb_ = cb;
        return DecodePNG(this, this, 0);
    }

    void PNG::decodeLine_(PNGDRAW *pDraw) {
        uint16_t line[320];
        PNG * png = reinterpret_cast<PNG*>(pDraw->pUser);
        PNGRGB565(pDraw, line, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff, png->iHasAlpha);
        png->cb_(reinterpret_cast<ColorRGB *>(line), pDraw->y, pDraw->iWidth);
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
    PNG::PNG() {
            memset(this, 0, sizeof(PNGIMAGE));
    }
#pragma GCC diagnostic pop



} // namespace rckid