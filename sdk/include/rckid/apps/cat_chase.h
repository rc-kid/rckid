#pragma once

#include <rckid/game/engine.h>
#include <rckid/game/sprite_set.h>
#include <rckid/game/sprite.h>

namespace rckid {

    /** A simple cat-chase game
     
        Serves as a demonstrator of the development ladder architecture. 
     */
    class CatChase : public game::Engine {
    public:
        CatChase():
            game::Engine("Cat") {
            catSprite_ = createAsset<game::SpriteSet>("CatSprite");
            catSprite_->addSprite(assets::icons_64::happy_face);

            mouseSprite_ = createAsset<game::SpriteSet>("MouseSprite");
            mouseSprite_->addSprite(assets::icons_64::footprint);

            cat_ = createObject<game::Sprite>("Cat");   
            cat_->setSpriteSet(catSprite_);
            cat_->setSpriteIndex(0);
            cat_->setPosition(Point{160 - 32, 120 - 32});

            mouse_ = createObject<game::Sprite>("Mouse");   
            mouse_->setSpriteSet(mouseSprite_);
            mouse_->setSpriteIndex(0);
            mouse_->setPosition(Point{20,20});

            btnUp()->onPressed += [this](){ cat_->moveBy(Point{0, -3}); };
            btnDown()->onPressed += [this](){ cat_->moveBy(Point{0, 3}); };
            btnLeft()->onPressed += [this](){ cat_->moveBy(Point{-3, 0}); };
            btnRight()->onPressed += [this](){ cat_->moveBy(Point{3, 0}); };
        }

    private:

        //game::Palette * palette_;
        game::SpriteSet * catSprite_;
        game::SpriteSet * mouseSprite_;
        game::Sprite * cat_;
        game::Sprite * mouse_;

        
    }; // rckid::GameEngine
}
