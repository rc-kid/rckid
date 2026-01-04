#pragma once

#include <optional>

#include "rckid.h"
#include "utils/stream.h"
#include "ui/menu.h"
#include "utils/ini.h"

namespace rckid {

    /** Base class for RCKid applications.
     
        Defines the application lifecycle and their stacking. 
     
     */
    class App {
    public:
        using MODAL_RESULT = void;

        static constexpr char const * LATEST_SLOT = "Latest";

        App():
            parent_{app_} {
        }

        virtual ~App() {
            if (app_ == this)
                app_ = nullptr;
        };

        virtual String title() const { return name(); }

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
        void saveState(String const & name);

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
        static typename T::MODAL_RESULT run(ARGS &&... args) {
            T app{std::forward<ARGS>(args)...};
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
        virtual bool isBudgeted() const { return false; }

        /** Returns true if the app supports saving and reloading its state (i.e. if the save and load methods are implemented). By default apps do not support state saving.
         */
        virtual bool supportsSaveState() const { return false; }

        /** Returns the current (active) application. May also return nullptr when the system code not managed by the App class (which should be exceedingly rare).
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

        /** Enters standalone app mode. 
         
            The standalone app mode diverts all resources to the currently running app. All background tasks are stopped and destroyed and any visual caches, such as background are cleared too, ensuring the current app has the most memory & cpu available to itself.
         */
        static void enableStandaloneMode();

    protected:

        /** Called when the application should gain focus. There can only be one focused app at a time. 
         */
        virtual void onFocus() { 
            ASSERT(app_ == nullptr);
            app_ = this;
        }

        /** Called when the application should loose focus. 
         
            When loosing focus the app should clear all its resources that can be easily recreated when focused again. 
         */
        virtual void onBlur() {
            ASSERT(app_ == this);
            app_ = nullptr;
        }

        /** Basic app update functionality. 
         
            In the simplest form, the update method checks home button press for the home menu which should be available from any app and checks the budget if applicable. The app expects the update method to be called after every frame.
         */
        virtual void update();

        /** Shows the main menu. 
         
            The method can be overriden to intercept the app change for main manu so that unnecessary memory can be freed, etc. By default, the method simply runs the home menu created by createHomeMenu().
         */
        virtual void showHomeMenu();

        /** Returns the home menu for the application. 
         
            By default, this generates the Exit action as well as load & save state options if state saves are supported. Note that this is then augmented by the non app-specific menu options added by the home menu itself.
         */
        virtual ui::ActionMenu * createHomeMenu();

        /** Method responsible for drawing the app contents on the screen. 
         
            While the method runs, 
         */
        virtual void draw() = 0;

        /** Exits the app. 
         
            The app does not exit immediately, but the next time its run method starts a new frame cycle. Overriding the method allows intercepting the app exit (but not cancelling the action). The default behavior is to save the game state into the _Latest_ slot, if the app supports save states.
         */
        virtual void exit() {
            if (supportsSaveState())
                saveState(LATEST_SLOT);
            exit_ = true;
        }

        /** Called every second. 
         */
        virtual void onSecondTick() {
            // do nothing by default
        }

        /** Current number of redraws. Reset automatically every second, should be incremented at each display redraw. 
         */
        static inline uint32_t redraws_ = 0;

        std::optional<ini::Writer> settingsWriter() {
            if (! fs::isMounted())
                return std::nullopt;
            fs::createFolders(homeFolder());
            String path = fs::join(homeFolder(), "settings.ini");
            return ini::Writer{fs::fileWrite(path)};
        }

        std::optional<ini::Reader> settingsReader() {
            if (! fs::isMounted())
                return std::nullopt;
            String path = fs::join(homeFolder(), "settings.ini");
            if (! fs::exists(path))
                return std::nullopt;
            return ini::Reader{fs::fileRead(path)};
        }

        /** Returns true if the app should exit. Useful for apps that provide their own main loop function.
         */
        bool shouldExit() const { return exit_; }

        /** Focuses the current app. 
         
            Ensures the proper app transition, blurs the parent, clears button states, waits for display update and then focuses itself. This function is useful for apps that provide their own main loop function.
         */
        void focus();

        /** Blurs the current app. 
         
            Blurs itself, waits for display update to finish, then clears the buttons state and focuses the parent app if any. This function is useful for apps that provide their own main loop function.
         */
        void blur();

        /** Power off handler. This should be called from the powerOff() function SDK implementation. Simply walks all the apps starting from the current app calls their exit() methods allowing them to react to the situation.
         */
        static void onPowerOff() {
            if (app_ == nullptr)
                return;
            app_->exit();
            App * x = app_->parent_;
            while (x != nullptr) {
                x->exit();
                x = x->parent_;
            }
        }

    private:

        friend void tick();
        friend void powerOff();

        static void secondTick();

        /** Verifies there is enough budget to play the app and returns true if ok.
         
            If the app is over budget, shows a dialog and causes the app to exit. If decrement is true, the system budget allowance will be decreased by 1 if the app is budgeted and there is budget (this is to be executed regularly every second).
         */
        bool verifyBudgetAllowance(bool decrement);

        App * parent_ = nullptr;
        bool exit_ = false;

        static inline App * app_ = nullptr;

        /** Falg that tells the update() method to check the app budget.  */
        static inline bool checkBudget_ = false;

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
        //MODAL_RESULT & result() { return result_; }

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

        void setResult(T result) {
            result_ = result;
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

        void onFocus() override {
            g_.initialize();
            App::onFocus();
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
