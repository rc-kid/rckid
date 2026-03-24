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

        Sprite(String name): Object{std::move(name)} {}

        Point position() const { return pos_; }

        void setPosition(Point value) { pos_ = value; }

        SpriteSet * spriteSet() const { return spriteSet_; }

        void setSpriteSet(SpriteSet * value) { 
            // TODO wrap around sprite set size
            spriteSet_ = value; 
        }

        Integer spriteIndex() const { return spriteIndex_; }
        void setSpriteIndex(Integer value) {
            // TODO wrap around actual size of the sprite set
            spriteIndex_ = value;
        }

        Palette * palette() const { return palette_; }
        void setPalette(Palette * value) {
            palette_ = value;
        }
        
        /** Renders the sprite. 
         */
        void render(Coord column, Color::RGB565 * buffer) override {
            if (! spriteSet_ || ! palette_)
                return;
            // check if the column is within the sprite bounds
            if (column < pos_.x || column >= pos_.x + spriteSet_->width())
                return;
            // adjust the row indices - offset the buffer data and determine how many pixels to draw
            Coord offset = pos_.y;
            Coord numPixels = spriteSet_->height();
            if (offset >= 0) {
                numPixels = std::min(numPixels, display::HEIGHT - offset);
                buffer += offset;
                offset = 0;
            } else {
                offset = - offset;
                numPixels = numPixels - offset;
            }
            // get the column from the spriteset and render it
            const Color::Index256 * spriteColumn = spriteSet_->getSpriteColumn(spriteIndex_, column - pos_.x);
            for (Coord y = 0; y < numPixels; y++) {
                Color::Index256 color = spriteColumn[offset + y];
                if (color.index() != 0)
                    buffer[y] = (*palette_)[color];
            }
        }

    private:
        Point pos_;
        Integer spriteIndex_ = 0;
        SpriteSet * spriteSet_;
        Palette * palette_;
    }; // rckid::game::Sprite

} // namespace rckid::game
