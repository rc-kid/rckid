#include "png.h"

// forward references
PNG_STATIC int PNGInit(PNGIMAGE *pPNG);
PNG_STATIC int DecodePNG(PNGIMAGE *pImage, void *pUser, int iOptions);
PNG_STATIC uint8_t PNGMakeMask(PNGDRAW *pDraw, uint8_t *pMask, uint8_t ucThreshold);
// Include the C code which does the actual work
#include "png.inl"

namespace rckid {

    PNG PNG::fromBuffer(uint8_t const * buffer, size_t numBytes) {
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



} // namespace rckid