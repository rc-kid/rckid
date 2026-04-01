#pragma once

#include <rckid/game/object.h>
#include <rckid/game/sprite_set.h>
#include <rckid/game/palette.h>
#include <rckid/game/script.h>
#include <rckid/game/descriptors.h>


namespace rckid::game {

    /** Sprite object.
     
        Sprites are images that can render themselves at given coordinates. Every sprite has its position and a pointer to the SpriteSet asset as well as the sprite index.
     */
    class Sprite : public Object {
    public:

        /** On collision event
         */
        using CollisionEvent = Event<Object *>;

        Sprite(String name, Engine * engine): 
            Object{std::move(name), engine} 
        {
            // default palette
            palette_ = engine->palette();
        }

        Point position() const { return pos_; }

        METHOD_DESCRIPTOR(position, assets::icons_24::bookmark,
            "Returns the position of the sprite",
            Type::Point(),
            ARGS(),
            CALL_WRAPPER((Object * obj, Value * args){
                static_cast<Sprite*>(obj)->position();
                return Value{};
            })
        );

        void setPosition(Point value) { pos_ = value; }

        METHOD_DESCRIPTOR(setPosition, assets::icons_24::bookmark, 
            "Sets position of the sprite",
            Type::Void(),
            ARGS(
                ARG(by, Type::Point(), assets::icons_24::bookmark, "New sprite position"),
            ),
            CALL_WRAPPER((Object * obj, Value * args) {
                static_cast<Sprite*>(obj)->setPosition(Point{0, 3});
                return Value{};
            })
        );


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

        void moveBy(Point by) {
            pos_ += by;
        }

        METHOD_DESCRIPTOR(moveBy, assets::icons_24::bookmark, 
            "Moves the sprite by given coordinates",
            Type::Void(),
            ARGS(
                ARG(by, Type::Point(), assets::icons_24::bookmark, "How much should the sprite move"),
            ),
            CALL_WRAPPER((Object * obj, Value * args) {
                static_cast<Sprite*>(obj)->moveBy(Point{0, 3});
                return Value{};
            })
        );

        void forceInRect(Rect rect = Rect::WH(display::WIDTH, display::HEIGHT)) {
            rect.w -= spriteSet_->width();
            rect.h -= spriteSet_->height();
            if (pos_.x < rect.left())
                pos_.x = rect.left();
            else if (pos_.x > rect.right())
                pos_.x = rect.right();
            if (pos_.y < rect.top())
                pos_.y = rect.top();
            else if (pos_.y > rect.bottom())
                pos_.y = rect.bottom();
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

        CollisionEvent onCollision;

        EVENT_DESCRIPTOR(onCollision, assets::icons_24::bookmark,
            "Fired when the sprite collides with another",
            ARGS(
                ARG(with, Type::Object(), assets::icons_24::bookmark, "The other sprite"),
            ),
            CONNECT_WRAPPER((Object * obj, std::function<void(Value *)> handler) {
                static_cast<Sprite*>(obj)->onCollision += [h = std::move(handler)](Object * with) {
                    Value v;
                    h(& v);
                };
            })
        );

    protected:

        void loop() override {
            if (onCollision.connected()) {
                // TODO check if sprites collide and emit if that is the case
            }
        }

    private:
        Point pos_;
        Integer spriteIndex_ = 0;
        SpriteSet * spriteSet_;
        Palette * palette_;

    public:
        CLASS_DESCRIPTOR(Sprite, assets::icons_24::bookmark,
            "Sprite with position and spritesheet that can move independently and collide with other sprites",
            PARENT(Object),
            CAPABILITIES(
                .renderable = true,
            ),
            METHODS(
                DESCRIPTOR(position),
                DESCRIPTOR(setPosition),
                DESCRIPTOR(moveBy),
            ),
            EVENTS(
                DESCRIPTOR(onCollision)
            )
        );

        ClassDescriptor const & typeDescriptor() const override { return descriptor; }

    }; // rckid::game::Sprite

} // namespace rckid::game
