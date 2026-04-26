#pragma once

#include <optional>

#include <rckid/graphics/bitmap.h>
#include <rckid/ui/wrapper.h>

namespace rckid::ui {

    /** Image widget is simply a wrapper widget around a bitmap.
     */
    using Image = Wrapper<Bitmap>;

    /** Custom fluent bitmap setter for Image. 
     */
    struct SetBitmap {
        Bitmap bitmap;
        SetBitmap(Bitmap bitmap): bitmap{std::move(bitmap)} {}
        SetBitmap(ImageSource src): bitmap{std::move(src)} {}
        template<size_t SIZE>
        SetBitmap(uint8_t const (& data)[SIZE]): bitmap{ImageSource{data}} {}

        SetBitmap && withoutTransparency() && {
            bitmap.setTransparentColor(std::nullopt);
            return std::move(*this);
        }
    };

    inline with<Image> operator << (with<Image> w, SetBitmap sb) {
        w->setContents(std::move(sb.bitmap));
        // only update the size if the image has not been resized yet
        if (w->width() == 0 || w->height() == 0)
            w->setRect(Rect::XYWH(w->position(), w->contents().width(), w->contents().height()));
        return w;
    }
} // namespace rckid::ui