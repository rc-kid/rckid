#pragma once

#include <vector>
#include <unordered_map>

#include <rckid/ui/app.h>

#include <rckid/game/script.h>
#include <rckid/game/object.h>
#include <rckid/game/asset.h>
#include <rckid/game/event.h>
#include <rckid/game/device.h>
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

            device_ = new Device{this};
            registerObject(device_);

            palette_ = createAsset<Palette>("Palette");
        }

        template<typename T>
        T * createAsset(String name) {
            // TODO static base of check
            T * result = new T{std::move(name), this};
            registerObject(result);
            return result;
        }

        Asset * createAsset(String const & className, String name) {
            meta::ClassDescriptor * cls = getClass(className);
            if (cls == nullptr)
                return nullptr;
            Object * obj = cls->create(std::move(name), this);
            registerObject(obj);
            return static_cast<Asset*>(obj);
        }

        template<typename T>
        T * createObject(String name) {
            // TODO static base of check
            T * result = new T{std::move(name), this};
            registerObject(result);
            return result;
        }

        // TODO add method for registering dynamically via just class name
            
        void declareFunction(Object const * object, meta::FunctionDescriptor * action) {

        }

        Device * device() { return device_; }

        Palette * palette() { return palette_; }

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

    private:

        /** Registers the given engine object into the dynamic runtime. 
        
            Note that the dynamic runtime does not always need to be present and is initialized lazily when the dynamic features are used. This means that if the game is writen purely in C++, we do not pay for the dynamic overhead even in memory.
         */
        void registerObject(Object * obj) {
            ObjectCapabilities caps = obj->capabilities();
            if (caps.renderable)
                renderableObjects_.push_back(unique_ptr<Object>{obj});
            else if (caps.passive)
                assets_.push_back(unique_ptr<Object>{obj});
            else
                nonRenderableObjects_.push_back(unique_ptr<Object>{obj});
        }

        // shorthands for common objects
        Device * device_;

        Palette * palette_;

        String gameName_;

        // game screen responsible for drawing all game elements
        GameScreen * screen_ = nullptr;

        // renderable game objects
        std::vector<unique_ptr<Object>> renderableObjects_;

        // non-renderable game objects (this is purely an optimization to avoid iterating over non-renderable objects in the render loop)
        std::vector<unique_ptr<Object>> nonRenderableObjects_;

        // game assets
        std::vector<unique_ptr<Object>> assets_;

        // game classes for reflection 
        std::unordered_map<String, unique_ptr<meta::ClassDescriptor>> objectClasses_;

    }; // rckid::GameEngine


} // namespace rckid
