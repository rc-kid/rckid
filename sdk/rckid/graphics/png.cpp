#include <PNGdec/src/PNGdec.h>

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

void closeStream(void * handle) {
    rckid::RandomReadStream * s = static_cast<rckid::RandomReadStream*>(handle);
    delete s;
}

typedef int32_t (PNG_SEEK_CALLBACK)(PNGFILE *pFile, int32_t iPosition);

namespace rckid {

    PNG::Decode16::Decode16(DecodeCallback16 cb, PNG * png): 
        cb{cb}, 
        png{png}, 
        line{Heap::alloc<uint16_t>(png->img_->iWidth)} {
    }

    PNG::Decode16::~Decode16() {
        delete [] line;
    }

    PNG PNG::fromStream(RandomReadStream * stream) {
        PNG result;
        result.img_->ucMemType = PNG_MEM_RAM;
        result.img_->pfnRead = readStream;
        result.img_->pfnSeek = seekStream;
        result.img_->pfnOpen = nullptr;
        result.img_->pfnClose = closeStream;
        result.img_->PNGFile.iSize = stream->size();
        result.img_->PNGFile.fHandle = stream;
        PNGInit(result.img_);
        return result;
    }

    PNG PNG::fromBuffer(uint8_t const * buffer, uint32_t numBytes) {
        PNG result{};
        result.img_->ucMemType = PNG_MEM_RAM;
        result.img_->pfnRead = readRAM;
        result.img_->pfnSeek = seekMem;
        result.img_->pfnOpen = nullptr;
        result.img_->pfnClose = nullptr;
        result.img_->PNGFile.iSize = numBytes;
        result.img_->PNGFile.pData = const_cast<uint8_t *>(buffer);
        PNGInit(result.img_);
        return result;
    } 

    Coord PNG::width() const { return img_->iWidth; }

    Coord PNG::height() const { return img_->iHeight; }

    bool PNG::decode16(DecodeCallback16 cb) {
        Decode16 d{cb, this};
        img_->pfnDraw = decodeLine16_;
        return DecodePNG(this->img_, & d, 0) == 0;
    }

    PNG::PNG() {
        img_ = new png_image_tag{};
        memset(img_, 0, sizeof(png_image_tag));
    }

    void PNG::decodeLine16_(png_draw_tag *pDraw) {
        Decode16 * dec = reinterpret_cast<Decode16*>(pDraw->pUser);
        //PNGRGB565(pDraw, line, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff, png->iHasAlpha);
        // TODO background is set to 0, which means the icons look good on black, but real transparency is not really working at this point
        PNGRGB565(pDraw, dec->line, PNG_RGB565_LITTLE_ENDIAN, 0x0, dec->png->img_->iHasAlpha);
        dec->cb(dec->line, pDraw->y, pDraw->iWidth);
    }

    PNG::~PNG() {
        if (img_->pfnClose != nullptr)
            img_->pfnClose(img_->PNGFile.fHandle);
        delete img_;
    }

}