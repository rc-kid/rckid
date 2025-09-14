#pragma once

#include <optional>

#include "rckid.h"
#include "utils/stream.h"
#include "ui/menu.h"

namespace rckid {

    /** Base class for RCKid applications.
     
        Defines the application lifecycle and their stacking. 
     
     */
    class App {
    public:
        using MODAL_RESULT = void;

        virtual ~App() = default;

        /** Returns the name of the app. 
            
            App name is used in various places, such as loading & saving states, etc. Every app must provide one and names of apps in the system should be unique.
         */
        virtual String name() const = 0;

        virtual void save([[maybe_unused]] WriteStream & into) {
            LOG(LL_ERROR, "Saving application state not supported");
        }   

        virtual void load([[maybe_unused]] ReadStream & from) {
            LOG(LL_ERROR, "Loading application state not supported");
        }

        /** Saves the app's state under the given name. Only works if the app supports the feature.
         */
        void saveState(String const & name) {
            LOG(LL_INFO, "Saving app state " << name);
            String folder = fs::join(homeFolder(), "saves");
            fs::createFolders(folder);
            fs::FileWrite f{fs::fileWrite(fs::join(folder, name))};
            save(f);
        }

        /** Loads state of given name. Only works if the app supports the feature.
         */
        void loadState(String const & name) {
            LOG(LL_INFO, "Loading app state " << name);
            String path = fs::join(fs::join(homeFolder(), "saves"), name);
            fs::FileRead f{fs::fileRead(path)};
            load(f);
        }

        /** Runs given application. Returns value if the application returns value. 
         */
        template<typename T, typename ... ARGS>
        static typename T::MODAL_RESULT run(ARGS ... args) {
            T app{args...};
            if (app.verifyBudgetAllowance(false))
                app.loop();
            return app.result();
        }

        /** Returns parent app, or nullptr currently root. 
         */
        App * parent() const { return parent_;}

        /** Returns the latest frames per second value. This is reset every second and if all goes well should be 60. As this is calculated by the app main loop itself, applications using different main loop strategies should indrement the number of redraws in their logic (see the redraw_ protected field).
         */
        static uint32_t fps() { return fps_; }

        /** Returns true if the current app should count towards the daily app time budget managed by the device. By default all apps count towards the budget, whereas some apps may decide otherwise.
        */
        virtual bool isBudgeted() const { return true; }

        /** Returns true if the app supports saving and reloading its state (i.e. if the save and load methods are implemented). By default apps do not support state saving.
         */
        virtual bool supportsSaveState() const { return false; }

        /** Returns the current (active) application. May also return nullptr during app transitions, or when the system code not managed by the App class (which should be exceedingly rare).
         */
        static App * currentApp() { return app_; }


        /** Returns the hole folder of the application. This is `/apps/AppName` on the SD card
         */
        String homeFolder() const {
            return fs::join("/apps", name());
        }

        /** Application main loop. Calling this method executes the application. 
         
            The loop method provides the logic for the periodic updates and drawing of the app. The method is called from the run() method of ModalApp class and should not be called directly.
         */
        virtual void loop();

        /** Returns the modal result of the application. 
         
            This method is overloaded in modal apps that actually might return something and the definition here only allows non-modal apps to be executed via the same code.
         */
        void result() const {}

    protected:

        /** Called when the application should gain focus. 
         
            There can only be one focused app at a time. When the app gains focus, it first blurs existing app. If there is existing app, it will become parent of the current app. Then the app resumes own state if suspended and finally sets itself as the focused app. 
         */
        virtual void focus() {
            // if parent is null, then this is new app that is replacing current app (if any). This is called from the run() loop so we should call blur of current app and set our parent to it. Then install ourselves as the current app. 
            if (parent_ == nullptr) {
                parent_ = app_;
                if (app_ != nullptr)
                    app_->blur();
            }
            // otherwise we are the current app and simply should focus ourselves, so continue with setting ourselves as the app and resuming
            app_ = this;
        }

        /** Called when the application should loose focus. 
         
            When loosing focus the app should clear all its resources that can be easily recreated when focused again. 
         */
        virtual void blur() {
            // there are two cases when blur occurs - when we are suspending current ap to launch a new child app, in which case the app_ points to the current app. This happens via focus() of the child and so we just blur ourselves and do nothing. Otherwise if the app is empty, we are closing this app and should terefore focus the parent app 
            if (app_ == nullptr)
                if (parent_ != nullptr)
                    parent_->focus();
        }

        /** */
        virtual void update();

        /** Overriding this function allows the app to specify its own items for the home menu. 
         
            The home menu for the application is created by calling the generator this function returns first, which is then updated by the general home menu options (exit, power off, brightness & volume, etc.)
         */
        virtual ui::ActionMenu::MenuGenerator homeMenuGenerator() {
            return [this](){ 
                ui::ActionMenu * m =  new ui::ActionMenu{}; 
                addDefaultHomeActionsInto(m);
                return m;
            };
        }

        /** Adds default app actions to given home menu. 
         
            This function is to be called from the custom home menu generators where it generates the default app actions, such as exit, or state loads & saves where applicable. 
         */
        void addDefaultHomeActionsInto(ui::ActionMenu * menu);

        /** Method responsible for drawing the app contents on the screen. 
         
            While the method runs, 
         */
        virtual void draw() = 0;

        /** Exits the app. The app does not exit immediately, but the next time its run method starts a new frame cycle. 
         */
        void exit() { app_ = nullptr; }

        /** Current number of redraws. Reset automatically every second, should be incremented at each display redraw. 
         */
        static inline uint32_t redraws_ = 0;

    private:

        friend void tick();

        static void onSecondTick();

        /** Verifies there is enough budget to play the app and returns true if ok.
         
            If the app is over budget, shows a dialog and causes the app to exit. If decrement is true, the system budget allowance will be decreased by 1 if the app is budgeted and there is budget (this is to be executed regularly every second).
         */
        bool verifyBudgetAllowance(bool decrement);

        App * parent_ = nullptr;

        static inline App * app_ = nullptr;

        static inline uint32_t fps_ = 0;

    }; // rckid::App

    template<typename T>
    class ModalApp : public App {
    public:
        using MODAL_RESULT = std::optional<T>;

        /** Runs the modal app in launcher mode, where when ready, the app calls the given callback function instead.
         */
        void run(std::function<void(T)> callback) {
            callback_ = std::move(callback);
            loop();
        }

        /** Returns the app's return value (if any).
         */
        MODAL_RESULT const & result() const { return result_; }
        MODAL_RESULT & result() { return result_; }

        using App::exit;

    protected:

        /** Exits the modal app with given result. 
         
            Note that in case the app is run in the callback mode, the app does not actually exit, but rather calls the callback function with the result and returns to the app logic afterwards.
         */
        void exit(T result) {
            if (callback_ == nullptr) {
                result_ = result;
                App::exit();
            } else {
                callback_(result);
            }
        }

    private:

        std::optional<T> result_;
        std::function<void(T)> callback_;
    }; // ModalApp

    template<>
    class ModalApp<void> : public App {
    public:
    }; // ModalApp<void>


    /** Application with Renderer that takes care of its rendering on the display.
     */
    template<typename RENDERER, typename T = void>
    class RenderableApp : public ModalApp<T> {
    protected:

        using ModalApp<T>::exit;
        template <typename... Args>
        RenderableApp(Args&&... args) : g_{std::forward<Args>(args)...} {
        }

        RenderableApp(RenderableApp const &) = delete;
        RenderableApp(RenderableApp &&) = delete;

        void focus() override {
            g_.initialize();
            App::focus();
        }

        void update() override {
            App::update();
            g_.update();
        }

        void draw() override {
            g_.render();
        }

        RENDERER g_;
    }; // rckid::RenderableApp



} // namespace rckid
