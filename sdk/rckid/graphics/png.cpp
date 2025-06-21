#include <PNGdec/src/PNGdec.h>

#include "png.h"


// forward references
//PNG_STATIC int PNGInit(PNGIMAGE *pPNG);
//PNG_STATIC int DecodePNG(PNGIMAGE *pImage, void *pUser, int iOptions);
//PNG_STATIC uint8_t PNGMakeMask(PNGDRAW *pDraw, uint8_t *pMask, uint8_t ucThreshold);

// Include the C code which does the actual work
//#include <PNGdec/src/png.inl>

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

    PNG::DecodeRGB::DecodeRGB(DecodeCallbackRGB cb, PNG * png): 
        cb{cb}, 
        png{png}, 
        line{Heap::alloc<uint16_t>(png->img_->iWidth)} {
    }

    PNG::DecodeRGB::~DecodeRGB() {
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

    uint32_t PNG::bpp() const { return PNG_getBpp(img_); }

    uint16_t * PNG::palette() const {
        uint8_t * p = PNG_getPalette(img_);
        if (p == nullptr)
            return nullptr;
        uint32_t numColors = 1 << bpp();
        uint16_t * result = new uint16_t[numColors];
        for (uint32_t i = 0; i < numColors; ++i) {
            result[i] = ColorRGB::RGB(
                p[i * 4], 
                p[i * 4 + 1],
                p[i * 4 + 2]
            ).raw16();
        }
        return result;
    }

    bool PNG::decodeRGB(DecodeCallbackRGB cb) {
        DecodeRGB d{cb, this};
        img_->pfnDraw = decodeLineRGB_;
        return DecodePNG(this->img_, & d, 0) == 0;
    }

    bool PNG::decode(DecodeCallback cb) {
        img_->pfnDraw = decodeLine_;
        return DecodePNG(this->img_, &cb, 0) == 0;
    }


    PNG::PNG() {
        img_ = new png_image_tag{};
        memset(img_, 0, sizeof(png_image_tag));
    }

    void PNG::decodeLineRGB_(png_draw_tag *pDraw) {
        DecodeRGB * dec = reinterpret_cast<DecodeRGB*>(pDraw->pUser);
        //PNGRGB565(pDraw, line, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff, png->iHasAlpha);
        // TODO background is set to 0, which means the icons look good on black, but real transparency is not really working at this point
        PNGRGB565(pDraw, dec->line, PNG_RGB565_LITTLE_ENDIAN, 0x0, dec->png->img_->iHasAlpha);
        dec->cb(dec->line, pDraw->y, pDraw->iWidth);
    }

    void PNG::decodeLine_(png_draw_tag * pDraw) {
        /*
        DecodeCallback * cb = reinterpret_cast<DecodeCallback*>(pDraw->pUser);
        uint16_t * pixels = Heap::alloc<uint16_t>(pDraw->iWidth);
        PNGRGB565(pDraw, pixels, PNG_RGB565_LITTLE_ENDIAN, 0x0, pDraw->iHasAlpha);
        (*cb)(pixels, pDraw->y, pDraw->iWidth);
        delete [] pixels;
        */
    }

    PNG::~PNG() {
        if (img_->pfnClose != nullptr)
            img_->pfnClose(img_->PNGFile.fHandle);
        delete img_;
    }

}