#include "png.h"

// forward references
PNG_STATIC int PNGInit(PNGIMAGE *pPNG);
PNG_STATIC int DecodePNG(PNGIMAGE *pImage, void *pUser, int iOptions);
PNG_STATIC uint8_t PNGMakeMask(PNGDRAW *pDraw, uint8_t *pMask, uint8_t ucThreshold);

WARNINGS_OFF
// Include the C code which does the actual work
#include <PNGdec/src/png.inl>
WARNINGS_ON

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
        result.img_->ucMemType = PNG_MEM_RAM;
        result.img_->pfnRead = readStream;
        result.img_->pfnSeek = seekStream;
        result.img_->pfnDraw = decodeLine_;
        result.img_->pfnOpen = nullptr;
        result.img_->pfnClose = nullptr;
        result.img_->PNGFile.iSize = stream.size();
        //result.PNGFile.pData = const_cast<uint8_t *>(buffer);
        result.img_->PNGFile.fHandle = & stream;
        PNGInit(result.img_);
        return result;
    }

    PNG PNG::fromBuffer(uint8_t const * buffer, uint32_t numBytes) {
        PNG result;
        result.img_->ucMemType = PNG_MEM_RAM;
        result.img_->pfnRead = readRAM;
        result.img_->pfnSeek = seekMem;
        result.img_->pfnDraw = decodeLine_;
        result.img_->pfnOpen = nullptr;
        result.img_->pfnClose = nullptr;
        result.img_->PNGFile.iSize = numBytes;
        result.img_->PNGFile.pData = const_cast<uint8_t *>(buffer);
        PNGInit(result.img_);
        return result;
    }

    int PNG::decode(DecodeCallback cb) {
        cb_ = cb;
        return DecodePNG(this->img_, this, 0);
    }

    void PNG::decodeLine_(PNGDRAW *pDraw) {
        uint16_t line[320];
        PNG * png = reinterpret_cast<PNG*>(pDraw->pUser);
        //PNGRGB565(pDraw, line, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff, png->iHasAlpha);
        // TODO background is set to 0, which means the icons look good on black, but real transparency is not really working at this point
        PNGRGB565(pDraw, line, PNG_RGB565_LITTLE_ENDIAN, 0x0, png->img_->iHasAlpha);
        png->cb_(reinterpret_cast<ColorRGB *>(line), pDraw->y, pDraw->iWidth);
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
    PNG::PNG() {
        img_ = new PNGIMAGE{};
        memset(img_, 0, sizeof(PNGIMAGE));
    }
#pragma GCC diagnostic pop



} // namespace rckid