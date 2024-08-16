#pragma once

#include "rckid.h"
#include "graphics/renderer.h"

namespace rckid {

    class App {
    public:

        virtual ~App() = default;

        /** Runs the app */
        void run(); 

        static uint32_t tickUs() { return tickUs_; }
        static uint32_t updateUs() { return updateUs_; }
        static uint32_t drawUs() { return drawUs_; }
        static uint32_t renderUs() { return renderUs_; }
        static uint32_t waitVSyncUs() { return waitVSyncUs_; }
        static uint32_t waitRenderUs() { return waitRenderUs_; }
        static uint32_t fps() { return fps_; }

    protected:

        /** App state update method. 
         
            
         */
        virtual void update() {
            if (btnPressed(Btn::B))
                exit();
        }

        virtual void draw() = 0;

        virtual void render() = 0;


        virtual void onFocus() {}
        virtual void onBlur() {}

        /** Exits from the app. 
         */
        void exit() {
            ASSERT(current_ == this);
            current_ = nullptr;
        }

    private:

        static inline App * current_ = nullptr;

        static inline uint32_t tickUs_ = 0;
        static inline uint32_t updateUs_ = 0;
        static inline uint32_t drawUs_ = 0;
        static inline uint32_t renderUs_ = 0;
        static inline uint32_t waitVSyncUs_ = 0;
        static inline uint32_t waitRenderUs_ = 0;
        static inline uint32_t fps_ = 0; 


    }; // rckid::App


    template<typename GRAPHICS>
    class GraphicsApp : public App {
    public:

    protected:
        GraphicsApp(GRAPHICS && g): g_{std::move(g) } {}

        void render() override {
            renderer_.render(g_);
        }

        void onFocus() override {
            App::onFocus();
            renderer_.initialize(g_);
        }

        void onBlur() override {
            App::onBlur();
            renderer_.finalize();
        }

        GRAPHICS g_;
        Renderer<GRAPHICS> renderer_;
    };

} // namespace rckid