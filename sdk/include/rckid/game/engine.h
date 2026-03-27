#pragma once

#include <vector>
#include <unordered_map>

#include <rckid/ui/app.h>

#include <rckid/game/script.h>
#include <rckid/game/object.h>
#include <rckid/game/asset.h>
#include <rckid/game/event.h>
#include <rckid/game/button.h>
#include <rckid/game/palette.h>

namespace rckid::game {

    class Asset;

    /** Game Engine
     
        Base class for game engine mechanics.

        - this is really an app, that runs the game. How the game is run depends on the objects it has, and on how the objects draw themselves, we can reuse the existing rendering pipelines for this
        - define the metadata about eventsm objects, etc 
        - the events

        - if objects are widgets, but not all objects are widgets, how to reconcile if I do not want to deal with multiple inheritance? 
     */
    class Engine : public ui::App<void> {
    public:

        String name() const override { return STR("GameEngine_" << gameName_); }

        Capabilities capabilities() const override {
            return {
                .canPersistState = true,
                .consumesBudget = false,
                .standalone = true,
            };
        }

        // forward declarations of game objects and their metadata        

        Engine(String gameName):
            gameName_{std::move(gameName)} 
        {
            screen_ = addChild(new GameScreen(this));
            btnUp_ = new Button{"ButtonUp", Btn::Up};
            btnDown_ = new Button{"ButtonDown", Btn::Down};
            btnLeft_ = new Button{"ButtonLeft", Btn::Left};
            btnRight_ = new Button{"ButtonRight", Btn::Right};
            btnA_ = new Button{"ButtonA", Btn::A};
            btnB_ = new Button{"ButtonB", Btn::B};
            btnSelect_ = new Button{"ButtonSelect", Btn::Select};
            btnStart_ = new Button{"ButtonStart", Btn::Start};

            registerEngineObject(btnUp_);
            registerEngineObject(btnDown_);
            registerEngineObject(btnLeft_);
            registerEngineObject(btnRight_);
            registerEngineObject(btnA_);
            registerEngineObject(btnB_);
            registerEngineObject(btnSelect_);
            registerEngineObject(btnStart_);

            palette_ = createAsset<Palette>("Palette");
        }

        template<typename T>
        T * createAsset(String name) {
            // TODO static base of check
            T * result = new T{std::move(name)};
            registerEngineObject(result);
            assets_.push_back(unique_ptr<T>{result});
            return result;
        }

        Asset * createAsset(String const & className, String name) {
            meta::ClassDescriptor * cls = getClass(className);
            if (cls == nullptr)
                return nullptr;
            EngineObject * obj = cls->create(std::move(name));
            registerEngineObject(obj);
            return static_cast<Asset*>(obj);
        }

        template<typename T>
        T * createObject(String name) {
            // TODO static base of check
            T * result = new T{std::move(name)};
            registerEngineObject(result);
            ObjectCapabilities caps = result->capabilities();
            if (caps.renderable)
                renderableObjects_.push_back(unique_ptr<T>{result});
            else 
                nonRenderableObjects_.push_back(unique_ptr<T>{result});
            return result;
        }

        // TODO add method for registering dynamically via just class name
            
        void declareFunction(Object const * object, meta::FunctionDescriptor * action) {

        }

    protected:

        meta::ClassDescriptor * getClass(String const & name) {
            auto i = objectClasses_.find(name);
            if (i == objectClasses_.end())
                return nullptr;
            return i->second.get();
        }

        /** The Game Screen widget responsible for rendering all of the rendarable game objects.
         */
        class GameScreen : public ui::Widget {
        public:

            GameScreen(Engine * engine): 
                engine_{engine} {
                setRect(Rect::WH(display::WIDTH, display::HEIGHT));
            }

            void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
                ASSERT(verifyRenderParams(width(), height(), column, startRow, numPixels));
                ASSERT(startRow == 0);
                ASSERT(numPixels == display::HEIGHT);
                // render all renderable objects
                for (auto & object : engine_->renderableObjects_)
                    object->render(column, buffer);
            }

        private:
            Engine * engine_;

        }; // Engine::GameScreen


        void loop() override {
            ui::App<void>::loop();
            for (auto & obj : renderableObjects_)
                obj->loop();
            for (auto & obj : nonRenderableObjects_)
                obj->loop();

        }

        Button * btnUp() { return btnUp_; }
        Button * btnDown() { return btnDown_; }
        Button * btnLeft() { return btnLeft_; }
        Button * btnRight() { return btnRight_; }
        Button * btnA() { return btnA_; }
        Button * btnB() { return btnB_; }
        Button * btnSelect() { return btnSelect_; }
        Button * btnStart() { return btnStart_; }

        Palette * palette() { return palette_; }
        
    private:

        /** Registers the given engine object into the dynamic runtime. 
        
            Note that the dynamic runtime does not always need to be present and is initialized lazily when the dynamic features are used. This means that if the game is writen purely in C++, we do not pay for the dynamic overhead even in memory.
         */
        void registerEngineObject(EngineObject * obj) {
            // TODO 
            

        }

        // shorthands for common objects
        Button * btnUp_;
        Button * btnDown_;
        Button * btnLeft_;
        Button * btnRight_;
        Button * btnA_;
        Button * btnB_;
        Button * btnSelect_;
        Button * btnStart_;

        Palette * palette_;

        String gameName_;

        // game screen responsible for drawing all game elements
        GameScreen * screen_ = nullptr;

        // renderable game objects
        std::vector<unique_ptr<Object>> renderableObjects_;

        // non-renderable game objects (this is purely an optimization to avoid iterating over non-renderable objects in the render loop)
        std::vector<unique_ptr<Object>> nonRenderableObjects_;

        // game assets
        std::vector<unique_ptr<Asset>> assets_;

        // game classes for reflection 
        std::unordered_map<String, unique_ptr<meta::ClassDescriptor>> objectClasses_;

    }; // rckid::GameEngine


} // namespace rckid
