#include <rckid/filesystem.h>
#include <rckid/ui/style.h>

namespace rckid::ui {

    void Style::loadDefaultStyle() {
        auto f = fs::readFile(Style::DEFAULT_FILE);
        if (f != nullptr) {
            ini::Reader reader{*f};
            load(reader);
        }
    }

    void Style::saveDefaultStyle() {
        auto f = fs::writeFile(Style::DEFAULT_FILE);
        if (f != nullptr) {
            ini::Writer writer{*f};
            save(writer);
        }
    }

    void Style::load(ini::Reader & reader) {
        backgroundImage_ = ImageSource{assets::images::logo};
        reader 
            >> ini::Section("default")
                >> ini::Field("fg", defaultFg_)
                >> ini::Field("bg", defaultBg_)
            >> ini::Section("accent")
                >> ini::Field("fg", accentFg_)
                >> ini::Field("bg", accentBg_)
            >> ini::Section("animation")
                >> ini::Field("speed", animationSpeed_)
            >> ini::Section("background")
                >> ini::Field("image", backgroundImage_);
    }

    void Style::save(ini::Writer & writer) {
        writer
            << ini::Section("default")
                << ini::Field("fg", defaultFg_)
                << ini::Field("bg", defaultBg_)
            << ini::Section("accent")
                << ini::Field("fg", accentFg_)
                << ini::Field("bg", accentBg_)
            << ini::Section("animation")
                << ini::Field("speed", animationSpeed_)
            << ini::Section("background")
                << ini::Field("image", backgroundImage_);
    }

} // namespace rckid::ui
