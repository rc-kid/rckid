#pragma once

namespace rckid {

    template<typename COLOR>
    class Canvas : public Bitmap<COLOR> {
    private:
        COLOR fg_;
        COLOR bg_;        

    }; // rckid::Canvas

} // namespace rckid