#include <PNGdec/src/PNGdec.h>

#include <rckid/graphics/png.h>

// PNG decode helpers

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

    PNGImageDecoder::PNGImageDecoder(unique_ptr<RandomReadStream> && stream):
        img_{new png_image_tag{}}
    {
        memset(img_, 0, sizeof(png_image_tag));
        img_->ucMemType = PNG_MEM_RAM;
        img_->pfnRead = readStream;
        img_->pfnSeek = seekStream;
        img_->pfnOpen = nullptr;
        img_->pfnClose = closeStream;
        img_->PNGFile.iSize = stream->size();
        img_->PNGFile.fHandle = stream.release();
        PNGInit(img_);
    }

    PNGImageDecoder::~PNGImageDecoder() {
        if (img_->pfnClose != nullptr)
            img_->pfnClose(img_->PNGFile.fHandle);
        delete img_;
    }

    Coord PNGImageDecoder::width() const {
        return img_->iWidth;
    }

    Coord PNGImageDecoder::height() const {
        return img_->iHeight;
    }

    Color::Representation PNGImageDecoder::colorRepresentation() const {
        switch (PNG_getPixelType(img_)) {
            case PNG_PIXEL_TRUECOLOR:
                return Color::Representation::RGB565;
            case PNG_PIXEL_INDEXED:
                switch (PNG_getBpp(img_)) {
                    case 8:
                        return Color::Representation::Index256;
                    case 4:
                        return Color::Representation::Index16;
                    default:
                        UNIMPLEMENTED;
                        return Color::Representation::RGB565;
                }
            default:
                UNIMPLEMENTED;
        }
        return Color::Representation::RGB565;
    }

    struct DecodeContext {
        Bitmap * bmp;
        unique_ptr<uint16_t> line;

        DecodeContext(Bitmap & bmp):
            bmp{& bmp} {
            if (bmp.colorRepresentation() == Color::Representation::RGB565)
                line = unique_ptr<uint16_t>(new uint16_t[bmp.width()]);
        }
    }; // DecodeContext

    Bitmap PNGImageDecoder::decode() {
        Bitmap result{width(), height(), colorRepresentation()};
        DecodeContext ctx{result};
        img_->pfnDraw = decodeLine_;
        DecodePNG(img_, & ctx, 0);
        return result;
    }

    void PNGImageDecoder::decodeLine_(png_draw_tag * pDraw) {
        DecodeContext * ctx = static_cast<DecodeContext*>(pDraw->pUser);
        switch (pDraw->iBpp) {
            case 16: {
                PNGRGB565(pDraw, ctx->line.get(), PNG_RGB565_LITTLE_ENDIAN, 0x0, pDraw->iHasAlpha);
                for (uint32_t x = 0; x < static_cast<uint32_t>(pDraw->iWidth); ++x)
                    ctx->bmp->setPixel(x, pDraw->y, static_cast<uint16_t>(ctx->line.get()[x]));
                break;
            }
            case 8: {
                switch (pDraw->iPixelType) {
                    case PNG_PIXEL_INDEXED: {
                        uint8_t * data = pDraw->pPixels;
                        for (uint32_t x = 0; x < static_cast<uint32_t>(pDraw->iWidth); ++x)
                            ctx->bmp->setPixel(x, pDraw->y, static_cast<uint16_t>(*(data++)));
                        break;
                    }
                    default:
                        UNIMPLEMENTED;
                }
                break;
            }
            case 4: {
                switch (pDraw->iPixelType) {
                    case PNG_PIXEL_INDEXED: {
                        uint8_t * data = pDraw->pPixels;
                        for (uint32_t x = 0; x < static_cast<uint32_t>(pDraw->iWidth); x += 2) {
                            uint8_t c = *(data++);
                            ctx->bmp->setPixel(x, pDraw->y, static_cast<uint16_t>((c >> 4)));
                            ctx->bmp->setPixel(x + 1, pDraw->y, static_cast<uint16_t>(c & 0xf));   
                        }
                        break;
                    }
                    default:
                        UNIMPLEMENTED;
                }
                break;
            }
            default:
                UNIMPLEMENTED;
        }
    }

}