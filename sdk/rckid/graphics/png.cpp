#include <PNGdec/src/PNGdec.h>

#include "png.h"

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
        result.line_ = new uint16_t[result.img_->iWidth];
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
        result.line_ = new uint16_t[result.img_->iWidth];
        return result;
    } 

    Coord PNG::width() const { return img_->iWidth; }

    Coord PNG::height() const { return img_->iHeight; }

    uint32_t PNG::bpp() const { 
        if (PNG_getPixelType(img_) == PNG_PIXEL_INDEXED)
            return PNG_getBpp(img_);
        else 
            return 16;
     }

    uint16_t * PNG::palette() const {
        uint8_t * p = PNG_getPalette(img_);
        if (p == nullptr)
            return nullptr;
        uint32_t numColors = 1 << bpp();
        uint16_t * result = new uint16_t[numColors];
        for (uint32_t i = 0; i < numColors; ++i) {
            result[i] = ColorRGB::RGB(
                p[i * 3], 
                p[i * 3 + 1],
                p[i * 3 + 2]
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
        Decode d{cb, this};
        img_->pfnDraw = decodeLine_;
        return DecodePNG(this->img_, & d, 0) == 0;
    }


    PNG::PNG() {
        img_ = new png_image_tag{};
        memset(img_, 0, sizeof(png_image_tag));
    }

    void PNG::decodeLineRGB_(png_draw_tag *pDraw) {
        DecodeRGB * dec = reinterpret_cast<DecodeRGB*>(pDraw->pUser);
        //PNGRGB565(pDraw, line, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff, png->iHasAlpha);
        // TODO background is set to 0, which means the icons look good on black, but real transparency is not really working at this point
        PNGRGB565(pDraw, dec->png->line_, PNG_RGB565_LITTLE_ENDIAN, 0x0, dec->png->img_->iHasAlpha);
        dec->cb(dec->png->line_, pDraw->y, pDraw->iWidth);
    }

    /** Do the right thing based on the bpp */
    void PNG::decodeLine_(png_draw_tag * pDraw) {
        Decode * dec = reinterpret_cast<Decode*>(pDraw->pUser);
        switch (pDraw->iBpp) {
            case 16: // 16bpp, convert to RGB565
                PNGRGB565(pDraw, dec->png->line_, PNG_RGB565_LITTLE_ENDIAN, 0x0, pDraw->iHasAlpha);
                break;
            case 8: // 8bpp, palette
                switch (pDraw->iPixelType) {
                    case PNG_PIXEL_INDEXED: { // indexed color
                        uint8_t * data = pDraw->pPixels;
                        for (uint32_t i = 0, e = static_cast<uint32_t>(pDraw->iWidth); i < e; ++i)
                            dec->png->line_[i] = *(data++);
                        break;
                    }
                    default:
                        UNIMPLEMENTED;
                }
                break;
            case 4: // 4bpp, palette
                switch (pDraw->iPixelType) {
                    case PNG_PIXEL_INDEXED: { // indexed color
                        uint8_t * data = pDraw->pPixels;
                        for (uint32_t i = 0, e = static_cast<uint32_t>(pDraw->iWidth); i < e; i += 2) {
                            uint8_t c = *(data++);
                            dec->png->line_[i] = (c >> 4);
                            dec->png->line_[i + 1] = (c & 0xf);   
                        }
                        break;
                    }
                    default:
                        UNIMPLEMENTED;
                }
                break;
            case 2: // 2bpp, palette
                UNIMPLEMENTED;
            case 1: // 1bpp, palette
                UNIMPLEMENTED;
        }
        dec->cb(dec->png->line_, pDraw->y, pDraw->iWidth);
    }

    PNG::~PNG() {
        if (img_->pfnClose != nullptr)
            img_->pfnClose(img_->PNGFile.fHandle);
        delete img_;
        delete line_;
    }

}