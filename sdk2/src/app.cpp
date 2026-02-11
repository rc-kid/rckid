#include <rckid/apps/home_menu.h>
#include <rckid/app.h>

namespace rckid {

    void App::run() {
        // if the app should exit already (was denied in constructor) do not even start the loop & focus transitions
        if (shouldExit_)
            return;
        // transition the focus and run the main loop
        if (current_ != nullptr)
            current_->onBlur();
        parent_ = current_;
        current_ = this;
        onFocus();
        onLoopStart();
        while (!shouldExit_) {
            tick();
            loop();
            render();
        }
        onBlur();
        current_ = parent_;
        // clear all button events so that the previous app can't react to them any more
        btnClearAll();
        // and call onFocus of the parent application
        if (current_ != nullptr)
            current_->onFocus();
    }

    void App::loop() {
        rckid::display::waitUpdateDone();
        if (! HomeMenu::active() && btnReleased(Btn::Home)) {
            HomeMenu app;
            auto action = app.run();
            if (action.has_value())
                action.value()();
        }
    }

    unique_ptr<ui::Menu> App::homeMenu() {
        auto result = std::make_unique<ui::Menu>();
        Capabilities caps = capabilities();
        // TODO add app specific home menu items here based on the capabilities
        // load & save state
        // take screenshot, etc.
        (*result)
            << ui::MenuItem("Exit", assets::icons_64::empty_box, [this]() { 
                exit(); 
            });
        if (caps.canPersistState) {
            // TODO generate load & save state menus here
        }
        if (caps.canCaptureScreen) {
            // TODO generate screenshot option
        }
        return result;
    }

    String App::homeFolder() const {
        return STR("/apps/" << name());
    }

    fs::Drive App::homeDrive() const {
        return fs::Drive::SD;
    }

    String App::resolvePath(String const & relativePath) const {
        return fs::join(homeFolder(), relativePath);
    }

    bool App::exists(String const & path) const {
        return fs::exists(resolvePath(path), homeDrive());
    }

    bool App::isFolder(String const & path) const {
        return fs::isFolder(resolvePath(path), homeDrive());
    }

    bool App::isFile(String const & path) const {
        return fs::isFile(resolvePath(path), homeDrive());
    }

    bool App::createFolder(String const & path) const {
        return fs::createFolder(resolvePath(path), homeDrive());
    }

    bool App::createFolders(String const & path) {
        return fs::createFolders(resolvePath(path), homeDrive());
    }

    bool App::eraseFile(String const & path) {
        return fs::eraseFile(resolvePath(path), homeDrive());
    }

    unique_ptr<RandomReadStream> App::readFile(String const & path) const {
        return fs::readFile(resolvePath(path), homeDrive());
    }

    unique_ptr<RandomWriteStream> App::writeFile(String const & path) {
        return fs::writeFile(resolvePath(path), homeDrive());
    }

    unique_ptr<RandomWriteStream> App::appendFile(String const & path) {
        return fs::appendFile(resolvePath(path), homeDrive());
    }

    uint32_t App::readFolder(String const & path, std::function<void(fs::FolderEntry const &)> callback) const {
        return fs::readFolder(resolvePath(path), homeDrive(), std::move(callback));
    }


} // namespace rckid