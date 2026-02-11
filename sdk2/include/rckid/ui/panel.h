#pragma once

#include <rckid/ui/widget.h>

namespace rckid::ui {

    class Panel : public Widget {
    public:

        void applyStyle(Style const * style, Theme theme) override {
            if (style == nullptr)
                return;
            Widget::applyStyle(style, theme);
            switch (theme) {
                case Theme::Default:
                    bg_ = style->defaultBg();
                    break;
                case Theme::Accent:
                    bg_ = style->accentBg();
                    break;
                case Theme::Info:
                    bg_ = style->infoBg();
                    break;
                case Theme::Success:
                    bg_ = style->successBg();
                    break;
                case Theme::Error:
                    bg_ = style->errorBg();
                    break;
                default:
                    UNREACHABLE;
            }
        }

        Color bg() const { return bg_; }

        void setBg(Color value) { bg_ = value; }

        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            memset16(reinterpret_cast<uint16_t*>(buffer), bg_.toRGB565(), numPixels);
            Widget::renderColumn(column, startRow, buffer, numPixels);
        }

    protected:

        Color bg_ = Color::RGB(0, 0, 0);

    }; // ui::Panel

    /** Sets panel's background color.
     */
    struct SetBg {
        Color color;
        SetBg(Color color): color{color} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetBg sb) {
        w->setBg(sb.color);
        return w;
    }

} // namespace rckid::ui