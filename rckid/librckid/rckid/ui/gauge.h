#pragma once

namespace rckid {

    /** Simple gauge with icon and name. 
     */
    template<typename COLOR>
    class Gauge {
    public:

        void configure(Bitmap<COLOR> const & icon, std::string const & text, int value, int min, int max) {
            icon_ = icon;
            text_ = text;
            value_ = value;
            min_ = min;
            max_ = max;
            textWidth_ = -1;

        }

        void drawOn(Canvas<COLOR> & canvas, Rect where) {
            if (textWidth == -1)
                textWidth = canvas.font().textWidth(text);

        }

        void drawOn(Canvas<COLOR> & canvas) {
            drawOn(canvas, Rect::WH(canvas.width(), canvas.height()));
        }


    private:

        Bitmap<COLOR> icon_{64,64};
        std::string text_;
        int textWidth_ = -1;
        int min_ = 0;
        int max_ = 100;
        int value_ = 50;
        int delta_ = 5;

    }; // rckid::Gauge

} // namespace rckid

