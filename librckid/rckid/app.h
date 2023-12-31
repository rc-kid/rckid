#pragma once

#include <vector>

#include "graphics/framebuffer.h"

namespace rckid {

    template<typename T>
    class App;

    /** Base class for all applications. 
     */
    class BaseApp {
    public:

        virtual ~BaseApp() = default;

        void run();
        void exit();

    protected:

        virtual void update() = 0;
        virtual void draw() = 0;
        virtual void render() = 0;
        virtual void onFocus(BaseApp * previous) {}
        virtual void onBlur(BaseApp * next) {}

        virtual bool takeRenderer(void * renderer, unsigned rendererId) { return false; }

        unsigned fps() const { return fps_; }
        unsigned systemUs() const { return systemUs_; }
        unsigned updateUs() const { return updateUs_; }
        unsigned drawUs() const { return drawUs_; }
        unsigned frameUs() const { return frameUs_; }
        unsigned idleUs() const { return frameUs_ - systemUs_ - updateUs_ - drawUs_; }
        unsigned idlePct() const { return idleUs() * 100 / frameUs_; }

    private:

        template<typename T> friend class App;

        static void loop_();

        static inline std::vector<BaseApp *> apps_;
        static inline BaseApp * currentApp_ = nullptr;

        static inline uint64_t nextFpsTick_;
        static inline unsigned fps_;
        static inline unsigned fpsCounter_;
        static inline unsigned systemUs_;
        static inline unsigned updateUs_;
        static inline unsigned drawUs_;
        static inline unsigned frameUs_;

    }; // rckid::BaseApp

    template<typename RENDERER>
    class App : public BaseApp {
    public:

        using Renderer = RENDERER;


    protected:

        void render() override {
            renderer_->startRendering();
        }

        void onFocus([[maybe_unused]] BaseApp * previous) {
            // if we do not have renderer, start it
            if (renderer_ == nullptr)
                renderer_ = new RENDERER{};
        }

        void onBlur(BaseApp * next) {
            // if next uses the same renderer, pass it, otherwise delete the renderer
            if (next == nullptr || next->takeRenderer(renderer_, RENDERER::RENDERER_ID))
                delete renderer_;
            renderer_ = nullptr;
        }

        RENDERER & renderer() { return * renderer_; }

    private:

        RENDERER * renderer_ = nullptr;

    }; // rckid::App<RENDERER>

} // namespace rckid