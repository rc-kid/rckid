#pragma once

#include <optional>
#include <rckid/rckid.h>
#include <rckid/string.h>
#include <rckid/filesystem.h>
#include <rckid/ui/menu.h>

namespace rckid {

    template<typename, typename = void>
    struct has_modal_result : std::false_type {};

    template<typename T>
    struct has_modal_result<T, std::void_t<typename T::MODAL_RESULT>> : std::true_type {};
    

    /** RCKid application. 
     
        Application is the code that controls the screen, reacts to the user input, and generally has the command of all of RCKid's resources. The SDK is designed to run a single application at any given time, but applications themselves can stack on top of another, so that the application on top has the control, which will return to the app below when the app exits.
        
        The base class provides common functionality for all apps, such as main loop, basic events and app lifecycle management. Specific apps derive from this class and implement their own extra logic. 

        Note that the App class exists only as a base class for the modal apps templated by their type to provide common base type irrespective of the result. Unless there is extremely complelling reason to do so, it should not be inherited directly (use ModalApp<> instead).
     */
    class App {
    public:

        /** Describes capabilities of the app. 
         
            Apps declare their capabilities via the capabilities() virtual method to allow the SDK determine what features and behaviors are expected from the application.  
         */
        class Capabilities {
        public:
            /** If true, the app is capable of loading and saving its state via the loadState and saveState methods. 
             */
            bool canPersistState = false;
            /** If true, the app can capture image of its current screen via the captureScreen method.
             */
            bool canCaptureScreen = false;
            /** If true, the app requires Bits (RCKid's budgeting currency) in order to run.
             */
            bool consumesBudget = false;
            /** If an app is standalone, it requires all available system resources. Starting such an app will cancel all current asks and tell all applications on the stack to release their resources via the releaseResources() method call. 
             */
            bool standalone = false;
        };

        /** Runs the application. 
         
            The run method is responsible for startingthe application and transferring focus between apps. First the newapplication is ceated, after which the old app is blurred, which allows the creation of the new application affect the blur method of the old one (such as when launcher's carousel is borrowed). Then the new app's onFocus is called, followed by onLoopStart. The loop of the application executes as long as the app does not call its exit function, after which first the onBlur method is called, followed by the destruction of the application (the only surviving thing from its time is the result, if any). Finally the onFocus of the old application is called as it becomes current. This is to ensure that all memory deallocations of the new app happen before the onFocus of the old one, which helps with memory fragmentation.
         */
        template<typename T, typename... ARGS>
        static typename T::MODAL_RESULT run(ARGS &&... args) {
            typename T::MODAL_RESULT result;
            {
                T app{std::forward<ARGS>(args)...};
                if (current_ != nullptr)
                    current_->onBlur();
                app.parent_ = current_;
                current_ = & app;
                app.enforceCapabilities();
                current_->onFocus();
                current_->onLoopStart();
                while (! app.shouldExit()) {
                    tick();
                    current_->loop();
                    current_->render();
                }
                current_->onBlur();
                current_ = current_->parent_;
                result = std::move(app.result());
            }
            btnClearAll();
            if (current_ != nullptr)
                current_->onFocus();
            return result;
        }

        /** Specialization of run method for apps that return void.
         
            See the non-void alternative for details.
         */
        template<typename T, typename...ARGS>
        static std::enable_if_t<! has_modal_result<T>::value, void> run(ARGS &&... args) {
            {
                T app{std::forward<ARGS>(args)...};
                if (current_ != nullptr)
                    current_->onBlur();
                app.parent_ = current_;
                current_ = & app;
                app.enforceCapabilities();
                current_->onFocus();
                current_->onLoopStart();
                while (! app.shouldExit()) {
                    tick();
                    current_->loop();
                    current_->render();
                }
                current_->onBlur();
                current_ = current_->parent_;
            }
            btnClearAll();
            if (current_ != nullptr)
                current_->onFocus();
        }

        virtual ~App() {
            ASSERT(current_ != this);
        }

        App * parent() const { return parent_; }

        /** Returns the name of the application. 
         
            Each application should have an unique name that is both used to visually identify the app to the user as well as a path to the app specific part of the filesystem for persistent data storage. The name *must* be unique across all apps.
         */
        virtual String name() const = 0;

        /** Returns the capabilities of the application.
         
            Default app behavior is *no* capabilities. Override this method in apps to declare supported or requires capabilities.
         */
        virtual Capabilities capabilities() const {
            return {};
        }

        /** Returns the currently running app. 
         */
        static App * current() { return current_; }

    protected:
        friend class HomeMenu; // for access to home menu generator

        /** Called when the application gains focus. 
         
            Default implementation is empty, and the method must be overriden for extra functionality (with base class onFocus() method called explicitly as well). A typical task for onFocus method is to prepare the screen for drawing, resume audio playback if any, etc.          
         */
        virtual void onFocus() {}

        /** Called when the application loses focus. 
 
            The default implementation is empty. Should be overriden in derived classes for extra functionality. Note that when overriden the base class's onBlur() *must* be called as well.
         */
        virtual void onBlur() {}

        /** Called when the application loop starts (and only if the application loop starts). 
         
            This function is a good place to start any animations that should be played when the app starts to preserve correct timing. Default implementation is empty.
         */
        virtual void onLoopStart();

        /** Main loop of the application. 
         
            This method is called repeatedly until exit() is called. The method should be overriden in derived classes to provide the actual application logic.
         */
        virtual void loop();

        /** Application rendering routine.
          
            Called by the run() method immediately after loop finishes. 
         */
        virtual void render() = 0;

        /** Loads app state from given stream. 
         
            Returns true if the state was loaded successfully, false otherwise. Override this method in apps that support state persistence. Note that the app must declare the canPersistState capability in order for the method to be called.

            The method can be called any time after the onLoopStart() method returns. 
         */
        virtual bool loadState([[maybe_unused]] RandomReadStream & stream) {
            UNREACHABLE;
            return false;
        }

        /** Saves app state to given stream. 
         
            Override this method in apps that support state persistence. Note that the app must declare the canPersistState capability in order for the method to be called.
            
            The method can be called any time after the onLoopStart() method returns.
         */
        virtual void saveState([[maybe_unused]] RandomWriteStream & stream) const {
            UNREACHABLE;
        }

        /** Convenience function that loads state from given filename (in the app's home folder).
         */
        void loadState(String filename);

        /** Convenience function that stores state to the given file in the app's home folder.
         */
        void saveState(String filename);

        /** Saves screen capture in any of the SDK supported image formats to the given stream.
         
            Override this method in apps that support screen capture. Note that the app must declare the canCaptureScreen capability in order for the method to be called. Returns true of the screen capture was successful, false otherwise.

            The method can be called any time after the onLoopStart() method returns.
         */
        virtual bool captureScreen([[maybe_unused]] RandomWriteStream & stream) const {
            UNREACHABLE;
            return false;
        }

        /** Called by the system when standalone app is about to be started. 
         
            When called, the app should release all of its resources (memory, file handles, HW resources, etc.) if supported in order to give the standalone app as much of the system available resources as possible. The default implementation is empty (no support). 
         */
        virtual void releaseResources() {
            if (parent_ != nullptr)
                parent_->releaseResources();
        }

        virtual unique_ptr<ui::Menu> homeMenu();

        /** Flags the application to exit. 
         
            Note that the app does not exit immediately, but rather after the end of its current loop() iteration. When app exists, the control is returned to the parent app, if any.

            If app supports state persistence, the state will be automatically saved to "Latest" slot in the app's home folder before exiting.
         */
        void exit() {
            if (capabilities().canPersistState)
                saveState("Latest");
            shouldExit_ = true;
        }

        bool shouldExit() const { return shouldExit_; }

        App() = default;

        /** \name Virtual filesystem access
         
            Each app has its own folder in which it can store its data. The functions below provide the identification of the app's home folder and drive on the actual filesystems as well as convenience wrappers around the filesystem function that automatically resolve the paths given to the app's home folder and drive.

            NOTE that the app's home folder is now always located on the SD card, but this can change in the future. 
         */
        //@{
        String homeFolder() const;

        fs::Drive homeDrive() const;

        bool homeDriveMounted() const;

        String resolvePath(String const & relativePath) const;

        bool exists(String const & path) const;

        bool isFolder(String const & path) const;

        bool isFile(String const & path) const;

        bool createFolder(String const & path) const;

        bool createFolders(String const & path);

        bool eraseFile(String const & path);

        unique_ptr<RandomReadStream> readFile(String const & path) const;

        unique_ptr<RandomWriteStream> writeFile(String const & path);

        unique_ptr<RandomWriteStream> appendFile(String const & path);

        uint32_t readFolder(String const & path, std::function<void(fs::FolderEntry const &)> callback) const;
        //@}

    private:

        // tick is friend so that it can force exit of the current app if budgeted
        friend void tick();

        void enforceCapabilities();

        App * parent_ = nullptr;
        // flag indicating whether the app should exit
        bool shouldExit_ = false;

        // currently running app
        static inline App * current_ = nullptr;

    }; // rckid::App

    /** App with result value 

        Allows the generic app to return an optional value at its completion (exit() method call). 
     */
    template<typename T>
    class ModalApp : public App {
    public:

        using MODAL_RESULT = std::optional<T>;

    protected:
        using App::exit;

        void exit(T && result) {
            result_ = std::move(result);
            exit();
        }

    private:
        friend class App;
        std::optional<T> result() { return std::move(result_);}

        std::optional<T> result_;

    }; // rckid::ModalApp<T>

    /** Specialization for apps that return nothing.
     */
    template<>
    class ModalApp<void> : public App {
    public:
    }; // rckid::ModalApp<void>

} // namespace rckid