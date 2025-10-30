#include "../utils/ini.h"
#include "../filesystem.h"
#include "header.h"
#include "form.h"

#include "style.h"

namespace rckid::ui {

    void Style::load() {
        fs::FileRead f{fs::fileRead(STYLE_SETTINGS_FILE)};
        if (! f.good()) {
            LOG(LL_INFO, "Style settings not found, using defaults");
            return;
        }
        ini::Reader reader{std::move(f)};
        while (auto section = reader.nextSection()) {
            if (section.value() == SECTION_DEFAULT) {
                while (auto o = reader.nextValue()) {
                    auto v = o.value();
                    if (v.first == "fg") {
                        fg_ = ColorRGB::fromString(v.second);
                    } else if (v.first == "bg") {
                        bg_ = ColorRGB::fromString(v.second);
                    } else {
                        LOG(LL_ERROR, "Unknown default style property " << v.first);
                    }
                }
            } else if (section.value() == SECTION_ACCENT) {
                while (auto o = reader.nextValue()) {
                    auto v = o.value();
                    if (v.first == "fg") {
                        accentFg_ = ColorRGB::fromString(v.second);
                    } else if (v.first == "bg") {
                        accentBg_ = ColorRGB::fromString(v.second);
                    } else {
                        LOG(LL_ERROR, "Unknown default style property " << v.first);
                    }
                }
            } else if (section.value() == SECTION_BACKGROUND) {
                while (auto o = reader.nextValue()) {
                    auto v = o.value();
                    if (v.first == "file") {
                        background_ = Icon{v.second.c_str()};
                    } else {
                        LOG(LL_ERROR, "Unknown background style property " << v.first);
                    }
                }
            } else {
                LOG(LL_ERROR, "Unknown style section " << section.value());
            }
        }
    }

    void Style::save() {
        ini::Writer writer{fs::fileWrite(STYLE_SETTINGS_FILE)};
        writer.writeSection(SECTION_DEFAULT);
        writer.writeValue("fg", fg_.toString());
        writer.writeValue("bg", bg_.toString());
        writer.writeSection(SECTION_ACCENT);
        writer.writeValue("fg", accentFg_.toString());
        writer.writeValue("bg", accentBg_.toString());
        writer.writeSection(SECTION_BACKGROUND);
        writer.writeValue("file", background_.filename() != nullptr ? String{background_.filename()} : ""); 
    }

    void Style::refresh() {
        Header::refreshStyle();
        FormWidget::refreshStyle();
    }

} // namespace rckid::ui