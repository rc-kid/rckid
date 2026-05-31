#pragma once

#include <rckid/game/engine.h>
#include <rckid/game/sprite_set.h>
#include <rckid/game/sprite.h>
#include <rckid/game/script.h>

namespace rckid {

    /** A simple cat-chase game
     
        Serves as a demonstrator of the development ladder architecture. 


     */
    class CatChase : public game::Engine {
    public:
        CatChase():
            game::Engine("Cat") {
            catSprite_ = createAsset<game::SpriteSet>("CatSprite");
            //catSprite_->addSprite(assets::icons_64::happy_face);

            mouseSprite_ = createAsset<game::SpriteSet>("MouseSprite");
            //mouseSprite_->addSprite(assets::icons_64::footprint);

            reloadAssets();

            cat_ = createObject<game::Sprite>("Cat");   
            cat_->setSpriteSet(catSprite_);
            cat_->setSpriteIndex(0);
            cat_->setPosition(Point{160 - 32, 120 - 32});

            mouse_ = createObject<game::Sprite>("Mouse");   
            mouse_->setSpriteSet(mouseSprite_);
            mouse_->setSpriteIndex(0);
            mouse_->setPosition(Point{20,20});

            device()->onButtonPressed += [this](Btn btn) {
                if (btn == Btn::Up)
                    cat_->moveBy(Point{0, -3});
            };

            device()->onButtonPressed += [this](Btn btn) {
                if (btn == Btn::Down)
                    cat_->moveBy(Point{0, 3});
            };

            device()->onButtonPressed += [this](Btn btn) {
                if (btn == Btn::Left)
                    cat_->moveBy(Point{-3, 0});
            };
            /*
            device()->onButtonPressed += [this](Btn btn) {
                if (btn == Btn::Right)
                    cat_->moveBy(Point{3, 0});
            };
            */
            game::ast::MethodCallNode * moveUp = new game::ast::MethodCallNode(
                std::make_unique<game::ast::ObjectNode>(cat_), 
                game::Sprite::descriptor.method("moveBy")
            );
            moveUp->addArgument(std::make_unique<game::ast::PointNode>(
                std::make_unique<game::ast::IntegerNode>(3), 
                std::make_unique<game::ast::IntegerNode>(0)
            ));
            game::Device::descriptor.event("onButtonPressed")->connect(game::as<game::Object>(device()), [moveUp, this](game::Value * args) {
                if (game::as<Btn>(args[0]) == Btn::Right) {
                    game::Evaluator::eval(moveUp, this);
                }
            });

            device()->onGameLoop += [this]() {
                switch (random() % 5) {
                    case 0 :
                        mouse_->moveBy(Point{0, -1});
                        break;
                    case 1:
                        mouse_->moveBy(Point{0, 1});
                        break;
                    case 2:
                        mouse_->moveBy(Point{-1, 0});
                        break;
                    case 3:
                        mouse_->moveBy(Point{1, 0});
                        break;
                    default:
                        break;
                }
                mouse_->forceInRect();
            };

            cat_->onCollision += [this](game::Object * other) {
                if (other == mouse_)
                    LOG(LL_INFO, "Yeah");
            };
        }

    protected:

        // TODO this is hacky function for demo, in reality we need proper dynamic asset management
        void reloadAssets() {
            catSprite_->clear();
            mouseSprite_->clear();

            catSprite_->addSprite(getSpriteBitmap("cat"));
            mouseSprite_->addSprite(getSpriteBitmap("mouse"));

        }

        Bitmap getSpriteBitmap(String name) {
            String fname = STR(homeFolder() << "/" << name << ".raw");
            if (fs::isFile(fname)) 
                return Bitmap{ImageSource{fname}};
            if (name == "cat")
                return Bitmap{ImageSource{assets::icons_64::happy_face}};
            else
                return Bitmap{ImageSource{assets::icons_64::footprint}};
        }

        /** Home menu of the game engine application. 
         
            This is basic home menu plus game engine actions, such as edits, etc.
         */
        unique_ptr<ui::Menu> homeMenu() override {
            auto m = ui::App<void>::homeMenu();
            m->insert(m->begin(), ui::MenuItem::Generator(
                "Edit", assets::icons_64::paint_palette,
                [this]() {
                    auto result = std::make_unique<ui::Menu>();
                    (*result)
                        << ui::MenuItem{
                            "Cat",
                            cat_->getIcon(),
                            [this](){
                                {
                                    Canvas canvas{getSpriteBitmap("cat")};
                                    App::run<Drawing>(& canvas);
                                    auto f = writeFile("cat.raw");
                                    canvas.saveAsRaw(*f.get());
                                }
                                reloadAssets();
                            }
                        }
                        << ui::MenuItem{
                            "Mouse",
                            mouse_->getIcon(),
                            [this](){
                                {
                                    Canvas canvas{getSpriteBitmap("mouse")};
                                    App::run<Drawing>(& canvas);
                                    auto f = writeFile("cat.raw");
                                    canvas.saveAsRaw(*f.get());
                                }
                                reloadAssets();
                                
                            }
                        };
                    return result;
                }
            ));
            return m;
        }

    private:

        game::SpriteSet * catSprite_;
        game::SpriteSet * mouseSprite_;
        game::Sprite * cat_;
        game::Sprite * mouse_;
        
    }; // rckid::GameEngine
}
