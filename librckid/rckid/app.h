#pragma once

#include <vector>

#include "ST7789.h"

namespace rckid {

    /** Base class for applications. */
    class BaseApp {
    public:

        virtual ~BaseApp() = default;
      
        void run();

        void exit();

    protected:

        virtual void onFocus() {}
        virtual void onBlur() {}
        virtual void update() = 0;
        virtual void draw() = 0;

        unsigned fps() const { return fps_; }
        unsigned systemUs() const { return systemUs_; }
        unsigned updateUs() const { return updateUs_; }
        unsigned drawUs() const { return drawUs_; }

    private:

        virtual void render_() {};

        static void loop_();

        static inline std::vector<BaseApp*> apps_;
        static inline BaseApp* currentApp_ = nullptr;

        static inline uint64_t nextFpsTick_;
        static inline unsigned fps_;
        static inline unsigned fpsCounter_;
        static inline unsigned systemUs_;
        static inline unsigned updateUs_;
        static inline unsigned drawUs_;
    }; 


    template<typename RENDERER>
    class App : public BaseApp {
    public:

        using Renderer = RENDERER;
        using Color = typename RENDERER::Color;

        ~App() override {
            if (parent_ == nullptr)
                delete renderer_;
        }

    protected: 

        App() = default;
        App(App * parent): renderer_{parent->renderer_}, parent_{parent} { parent->sharedWithChild_ = true; }

        void onFocus() {
            if (renderer_ == nullptr)
                createRenderer();
            // when focused, the app is on top and hence its renderer cannot be shared with child
            sharedWithChild_ = false;
        }

        void onBlur() {
            if (!sharedWithChild_)
                deleteRenderer();
        }

        virtual void draw(RENDERER & r) = 0;

    private:

        void draw() override { 
            ST7789::waitUpdateDone();
            draw(*renderer_);
            renderer_->startRendering();
        }

    private:

        void createRenderer() {
            // create new renderer
            renderer_ = new RENDERER{};
            renderer_->configureDisplay();
            // and propagate it to all parents of the same kind
            auto i = this->parent_;
            while (i != nullptr) {
                i->renderer_ = renderer_;
                i = i->parent_;
            }
        }

        void deleteRenderer() {
            // delete the renderer
            delete renderer_;
            // and propagate the removal to all parents of the same renderer kind
            auto i = this;
            while (i != nullptr) {
                i->renderer_ = nullptr;
                i = i->parent_;
            }
        }

        RENDERER * renderer_ = nullptr;
        App<RENDERER> * parent_ = nullptr;
        bool sharedWithChild_ = false;

    }; 






} // namespace rckid