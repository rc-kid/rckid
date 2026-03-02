
#include <rckid/apps/home_menu.h>
#include <rckid/apps/dialogs/info_dialog.h>
#include <rckid/task.h>
#include <rckid/app.h>

namespace rckid {

    void App::onLoopStart() {
        ui::Header::update();
    }

    void App::loop() {
        rckid::display::waitUpdateDone();
        if (! HomeMenu::active() && btnReleased(Btn::Home)) {
            auto action = App::run<HomeMenu>();
            if (action.has_value())
                action.value()();
        }
        if (btnPressed(Btn::VolumeUp))
            audio::setVolume(audio::volume() + 1);
        if (btnPressed(Btn::VolumeDown))
            audio::setVolume(audio::volume() - 1);
    }

    unique_ptr<ui::Menu> App::homeMenu() {
        auto result = std::make_unique<ui::Menu>();
        Capabilities caps = capabilities();
        // TODO add app specific home menu items here based on the capabilities
        // load & save state
        // take screenshot, etc.
        if (caps.canPersistState) {
            (*result) << ui::MenuItem::Generator("Load", assets::icons_64::bookmark, [this]() {
                auto result = std::make_unique<ui::Menu>();
                readFolder("saves", [this, & result] (fs::FolderEntry const & entry) {
                    if (entry.isFolder)
                        return;
                    (*result) << ui::MenuItem(fs::stem(entry.name), assets::icons_64::bookmark, [this, entryName = entry.name]() {
                        loadState(entryName);
                    });
                });
                return result;
            });
            (*result) << ui::MenuItem::Generator("Save", assets::icons_64::bookmark, [this]() {
                auto result = std::make_unique<ui::Menu>();
                for (uint32_t i = 1; i <= 4; ++i) {
                    String slotName = STR(i);
                    (*result) << ui::MenuItem(slotName, assets::icons_64::bookmark, [this, slotName]() {
                        saveState(slotName);
                    });
                }
                return result;
            });
        }
        if (caps.canCaptureScreen) {
            // TODO generate screenshot option
        }
        return result;
    }

    void App::loadState(String filename) {
        filename = fs::join("saves", filename);
        auto f = readFile(filename);
        if (f == nullptr)
            return InfoDialog::error("Cannot load state", STR("Unable to open file " << filename));
        if (! loadState(*f))
            return InfoDialog::error("Cannot load state", STR("Corrupted save file " << filename));
    }

    void App::saveState(String filename) {
        filename = fs::join("saves", filename);
        auto f = writeFile(filename);
        if (f == nullptr)
            return InfoDialog::error("Cannot save state", STR("Unable to open file " << filename));
        saveState(*f);
    }

    String App::homeFolder() const {
        return fs::join("/apps", name());
    }

    fs::Drive App::homeDrive() const {
        return fs::Drive::SD;
    }

    bool App::homeDriveMounted() const {
        return fs::isMounted(homeDrive());
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

    void App::enforceCapabilities() {
        Capabilities caps = capabilities();
        if (caps.consumesBudget && pim::remainingBudget() == 0) {
            InfoDialog::error("Out of budget", "No more budget today to play the game.");
            return exit();
        }
        if (homeDriveMounted()) {
            if (caps.canPersistState)
                createFolders("saves");
            if (caps.canCaptureScreen)
                createFolders("screenshots");
        }
        if (caps.standalone) {
            if (current_->parent_ != nullptr)
               current_->parent_->releaseResources();
            Task::releaseTaskResources();
        }
        // TODO other capabilities

    }


} // namespace rckid