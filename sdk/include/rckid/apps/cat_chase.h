#pragma once

#include <rckid/game/engine.h>
#include <rckid/game/sprite_set.h>
#include <rckid/game/sprite.h>

namespace rckid {

    class CatChase : public game::Engine {
    public:
        CatChase():
            game::Engine("Cat") {
            catSprite_ = createAsset<game::SpriteSet>("CatSprite");
            catSprite_->addSprite(assets::icons_64::happy_face);

            mouseSprite_ = createAsset<game::SpriteSet>("MouseSprite");
            mouseSprite_->addSprite(assets::icons_64::footprint);

            // TODO I am unhappy the palette has to be created like this, would be better automatic, but how to initialize? 
            palette_ = createAsset<game::Palette>("Palette");

            cat_ = createObject<game::Sprite>("Cat");   
            cat_->setSpriteSet(catSprite_);
            cat_->setSpriteIndex(0);
            cat_->setPosition(Point{160 - 32, 120 - 32});
            cat_->setPalette(palette_);

            mouse_ = createObject<game::Sprite>("Mouse");   
            mouse_->setSpriteSet(mouseSprite_);
            mouse_->setSpriteIndex(0);
            mouse_->setPosition(Point{20,20});
            mouse_->setPalette(palette_);
        }

    private:
        game::Palette * palette_;
        game::SpriteSet * catSprite_;
        game::SpriteSet * mouseSprite_;
        game::Sprite * cat_;
        game::Sprite * mouse_;

    }; // rckid::GameEngine
}
