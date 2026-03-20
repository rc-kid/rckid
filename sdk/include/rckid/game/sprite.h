#pragma once

#include <rckid/game/object.h>
#include <rckid/game/sprite_set.h>
#include <rckid/game/palette.h>

namespace rckid::game {

    /** Sprite object.
     
        Sprites are images that can render themselves at given coordinates. Every sprite has its position and a pointer to the SpriteSet asset as well as the sprite index.
     */
    class Sprite : public Object {
    public:
        char const * className() const override { return "Sprite"; }    

        /** Renders the sprite. 
         */
        void render(Coord column, Color::RGB565 * buffer) override {
            if (! spriteSet_ || ! palette_)
                return;
            // check if the column is within the sprite bounds
            if (column < pos_.x || column >= pos_.x + spriteSet_->width())
                return;
            // get the column from the spriteset and render it
            const Color::Index256 * spriteColumn = spriteSet_->getSpriteColumn(spriteIndex_, column - pos_.x);
            for (Coord row = 0; row < spriteSet_->height(); row++) {
                Color::Index256 color = spriteColumn[row];
                if (color.index() != 0)
                    buffer[row] = (*palette_)[color];
            }
        }

    private:
        Point pos_;
        Integer spriteIndex_ = 0;
        SpriteSet spriteSet_;
        Palette palette_;
    }; // rckid::game::Sprite

} // namespace rckid::game