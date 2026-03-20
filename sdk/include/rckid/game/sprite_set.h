#pragma once

#include <rckid/memory.h>
#include <rckid/graphics/color.h>
#include <rckid/game/engine.h>
#include <rckid/game/asset.h>

namespace rckid::game {

    /** Spriteset asset. 
     
        Spriteset is a collection of sprites that all have the same size. Spritesets are commonly used to store animations as a sequence of sprites. 
     */
    class SpriteSet : public Asset {
    public:

        SpriteSet() = default;

        SpriteSet(Integer width, Integer height, Integer size = 1):
            width_{width}, 
            height_{height}, 
            size_{size},
            sprites_{new Color::Index256*[size]} {
            while (size-- > 0)
                sprites_[size] = new Color::Index256[width * height];
        }

        SpriteSet(Bitmap bitmap):
            width_{bitmap.width()},
            height_{bitmap.height()},
            size_{0} {
            addSprite(std::move(bitmap));
            ASSERT(size_ == 1);
        }

        ~SpriteSet() override {
            for (Integer i = 0; i < size_; i++)
                delete [] sprites_[i];
            delete [] sprites_;
        }

        /** Width of each sprite in the set.
         */
        Integer width() const { return width_; }

        /** Height of each sprite in the set. 
         */
        Integer height() const { return height_; }

        /** Number of sprites in the set.  
         */
        Integer size() const { return size_; }

        /** Returns the specific column for given sprite index so that it can be rendered. 
         */
        Color::Index256 const * getSpriteColumn(Integer spriteIndex, Integer column) const {
            ASSERT(spriteIndex >= 0 && spriteIndex < size_);
            ASSERT(column >= 0 && column < width_);
            return sprites_[spriteIndex] + mapIndexColumnFirst(column, 0, width_, height_);
        }

        /** Adds new empty sprite. 
         */
        void addSprite() {
            ASSERT(width_ > 0 && height_ > 0);
            addSprite(new Color::Index256[width_ * height_]);
        }

        /** Adds new sprite taken from the given bitmap. 
         
            The bitmap must have proper width and height. If the bitmap depth equals to 8, which is used in the game engine the bitmap is moved, otherwise the image is recoded to 256 colors.
         */
        void addSprite(Bitmap bitmap) {
            ASSERT(bitmap.width() == width() && bitmap.height() == height());
            switch (bitmap.colorRepresentation()) {
                case Color::Representation::RGB565:
                case Color::Representation::RGB332:
                    // TODO determine the nearest color from a palette? 
                    UNIMPLEMENTED;
                    break;
                case Color::Representation::Index256:
                    addSprite(reinterpret_cast<Color::Index256 *>(std::move(bitmap).detachPixelArray()));
                    break;
                case Color::Representation::Index16:
                    // TODO copy color by color
                    UNIMPLEMENTED;
                    break;
                default:
                    UNREACHABLE;
            }
        }

    private:

        void addSprite(Color::Index256 * spriteData) {
            Color::Index256 ** old_ = sprites_;
            sprites_ = new Color::Index256**[size_ + 1];
            for (Integer i = 0 ; i < size_; ++i)
                sprites_[i] = old_[i];
            sprites_[size_] = spriteData;
            ++size_;
            delete [] old_;
        }


        Integer width_ = 0;
        Integer height_ = 0;
        Integer size_ = 0;

        Color::Index256 ** sprites_ = nullptr;

    }; // rckid::game::SpriteSet

} // namespace rckid::game