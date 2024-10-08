#pragma once

#include <optional>

#include "rckid.h"
#include "graphics/drawing.h"

namespace rckid {


    /** Template metaprogramming trick to determine whether ModalResult is defined on a given app type.
     */
    template<typename, typename = void>
    constexpr bool HasModalResult = false;

    template<typename T>
    constexpr bool HasModalResult
        <T, std::void_t<decltype(sizeof(typename T::ModalResult))>> = true;

    class App {
    public:

        virtual ~App() noexcept = default;

        static uint32_t tickUs() { return tickUs_; }
        static uint32_t updateUs() { return updateUs_; }
        static uint32_t drawUs() { return drawUs_; }
        static uint32_t renderUs() { return renderUs_; }
        static uint32_t waitVSyncUs() { return waitVSyncUs_; }
        static uint32_t waitRenderUs() { return waitRenderUs_; }
        static uint32_t fps() { return fps_; }

    protected:

        /** Runs the app's main loop until exit() is called.
         */
        void loop(); 

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

        virtual void pause();

        /** Exits from the app. 
         */
        void exit() {
            ASSERT(current_ == this);
            current_ = nullptr;
        }

        template<typename T> 
        typename std::enable_if<HasModalResult<T>, std::optional<typename T::ModalResult>>::type
        runModal() {
            onBlur();
            //memoryEnterArena();
            auto result = T::run();
            //memoryLeaveArena();
            onFocus();
            return result;
        }

        template<typename T>
        typename std::enable_if<!HasModalResult<T>, void>::type 
        runModal() {
            onBlur();
            memoryEnterArena();
            T::run();
            memoryLeaveArena();
            onFocus();
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
        using Color = typename GRAPHICS::Color;

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