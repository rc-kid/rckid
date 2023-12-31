#pragma once

#include <vector>

//#include "graphics/framebuffer.h"
#include "ST7789.h"

#include "graphics/png.h"
#include "fonts/Iosevka_Mono6pt7b.h"


namespace rckid {

    template<typename T>
    class App;

    /** Base class for all applications. 
     
        Defines basic application API and implements the main loop and navigation stack. RCKid runs a single app at any given time, but apps may form a stack so that exitting the top app resumes execution of the app that launched it. 
     */
    class BaseApp {
    public:

        virtual ~BaseApp() = default;

        void run();
        void exit();



        static PNG imgX() { return PNG::fromBuffer(Img_, sizeof(Img_)); }

        virtual PNG img() { return PNG::fromBuffer(Img_, sizeof(Img_)); }
        virtual char const * name() { return "App"; }

        static unsigned fps() { return fps_; }
        static unsigned systemUs() { return systemUs_; }
        static unsigned updateUs() { return updateUs_; }
        static unsigned drawUs() { return drawUs_; }
        static unsigned frameUs() { return frameUs_; }
        static unsigned idleUs() { return frameUs_ - systemUs_ - updateUs_ - drawUs_; }
        static unsigned idlePct() { return idleUs() * 100 / frameUs_; }


    protected:

        virtual void update() = 0;
        virtual void draw() = 0;
        virtual void render() = 0;
        virtual void onFocus(BaseApp * previous) {}
        virtual void onBlur(BaseApp * next) {}

        virtual bool takeRenderer(void * renderer, unsigned rendererId) { return false; }

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

        static constexpr uint8_t const Img_[] = {
#include "images/applications.png.raw"
        };

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