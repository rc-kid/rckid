#include "../utils/ini.h"
#include "../filesystem.h"
#include "header.h"
#include "form.h"

#include "style.h"

namespace rckid::ui {

    void Style::showRGBStyle() {
        switch (rgbStyle_) {
            case RGBStyle::Off:
                rgbOff();
                break;
            case RGBStyle::Rainbow:
                rckid::rgbEffects(
                    RGBEffect::Rainbow(0, 1, 4, rgbBrightness_), 
                    RGBEffect::Rainbow(0, 1, 4, rgbBrightness_), 
                    RGBEffect::Rainbow(0, 1, 4, rgbBrightness_), 
                    RGBEffect::Rainbow(0, 1, 4, rgbBrightness_), 
                    RGBEffect::Rainbow(0, 1, 4, rgbBrightness_)
                );
                break;
            case RGBStyle::RainbowWave:
                rckid::rgbEffects(
                    RGBEffect::Rainbow(0, 1, 4, rgbBrightness_), 
                    RGBEffect::Rainbow(51, 1, 4, rgbBrightness_), 
                    RGBEffect::Rainbow(102, 1, 4, rgbBrightness_), 
                    RGBEffect::Rainbow(153, 1, 4, rgbBrightness_), 
                    RGBEffect::Rainbow(204, 1, 4, rgbBrightness_)
                );
                break;
            case RGBStyle::Breathe:
                rckid::rgbEffects(
                    RGBEffect::Breathe(platform::Color::RGB(rgbColor_.r(), rgbColor_.g(), rgbColor_.b()).withBrightness(rgbBrightness_)),
                    RGBEffect::Breathe(platform::Color::RGB(rgbColor_.r(), rgbColor_.g(), rgbColor_.b()).withBrightness(rgbBrightness_)),
                    RGBEffect::Breathe(platform::Color::RGB(rgbColor_.r(), rgbColor_.g(), rgbColor_.b()).withBrightness(rgbBrightness_)),
                    RGBEffect::Breathe(platform::Color::RGB(rgbColor_.r(), rgbColor_.g(), rgbColor_.b()).withBrightness(rgbBrightness_)),
                    RGBEffect::Breathe(platform::Color::RGB(rgbColor_.r(), rgbColor_.g(), rgbColor_.b()).withBrightness(rgbBrightness_))
                );
                break;
            case RGBStyle::BreatheWave:
                // TODO breathe wave does not work yet, will not be wave
                rckid::rgbEffects(
                    RGBEffect::Breathe(platform::Color::RGB(rgbColor_.r(), rgbColor_.g(), rgbColor_.b()).withBrightness(rgbBrightness_)),
                    RGBEffect::Breathe(platform::Color::RGB(rgbColor_.r(), rgbColor_.g(), rgbColor_.b()).withBrightness(rgbBrightness_)),
                    RGBEffect::Breathe(platform::Color::RGB(rgbColor_.r(), rgbColor_.g(), rgbColor_.b()).withBrightness(rgbBrightness_)),
                    RGBEffect::Breathe(platform::Color::RGB(rgbColor_.r(), rgbColor_.g(), rgbColor_.b()).withBrightness(rgbBrightness_)),
                    RGBEffect::Breathe(platform::Color::RGB(rgbColor_.r(), rgbColor_.g(), rgbColor_.b()).withBrightness(rgbBrightness_))
                );
                break;
            case RGBStyle::Solid:
                rckid::rgbEffects(
                    RGBEffect::Solid(platform::Color::RGB(rgbColor_.r(), rgbColor_.g(), rgbColor_.b()).withBrightness(rgbBrightness_)),
                    RGBEffect::Solid(platform::Color::RGB(rgbColor_.r(), rgbColor_.g(), rgbColor_.b()).withBrightness(rgbBrightness_)),
                    RGBEffect::Solid(platform::Color::RGB(rgbColor_.r(), rgbColor_.g(), rgbColor_.b()).withBrightness(rgbBrightness_)),
                    RGBEffect::Solid(platform::Color::RGB(rgbColor_.r(), rgbColor_.g(), rgbColor_.b()).withBrightness(rgbBrightness_)),
                    RGBEffect::Solid(platform::Color::RGB(rgbColor_.r(), rgbColor_.g(), rgbColor_.b()).withBrightness(rgbBrightness_))
                );
                break;
            case RGBStyle::Key:
            case RGBStyle::RainbowKey:
                // show solid black color so that we do not turn the RGB voltage off which would add latency
                rckid::rgbEffects(
                    RGBEffect::Solid(platform::Color::Black()),
                    RGBEffect::Solid(platform::Color::Black()),
                    RGBEffect::Solid(platform::Color::Black()),
                    RGBEffect::Solid(platform::Color::Black()),
                    RGBEffect::Solid(platform::Color::Black())
                );
                break;
        }
    }

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
                    } else if (v.first == "tilt") {
                        backgroundTilt_ = (v.second == "1" || v.second == "true" || v.second == "yes");
                    } else if (v.first == "scroll") {
                        backgroundScroll_ = backgroundScrollStyleFromString(v.second);
                    } else {
                        LOG(LL_ERROR, "Unknown background style property " << v.first);
                    }
                }
            } else if (section.value() == SECTION_RGB) {
                while (auto o = reader.nextValue()) {
                    auto v = o.value();
                    if (v.first == "brightness") {
                        rgbBrightness_ = std::atoi(v.second.c_str());
                    } else if (v.first == "style") {
                        rgbStyle_ = rgbStyleFromString(v.second);
                    } else if (v.first == "color") {
                        rgbColor_ = ColorRGB::fromString(v.second);
                    } else {
                        LOG(LL_ERROR, "Unknown rgb style property " << v.first);
                    }
                }
            } else if (section.value() == SECTION_RUMBLER) {
                while (auto o = reader.nextValue()) {
                    auto v = o.value();
                    if (v.first == "intensity") {
                        rumblerKeyPressIntensity_ = std::atoi(v.second.c_str());
                    } else {
                        LOG(LL_ERROR, "Unknown rumbler style property " << v.first);
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
        writer.writeValue("tilt", backgroundTilt_ ? "true" : "false");
        writer.writeValue("scroll", STR(backgroundScroll_));
        writer.writeSection(SECTION_RGB);
        writer.writeValue("brightness", rgbBrightness());
        writer.writeValue("style", STR(rgbStyle()));
        writer.writeValue("color", rgbColor().toString());
        writer.writeSection(SECTION_RUMBLER);
        writer.writeValue("intensity", rumblerKeyPressIntensity());
    }

    void Style::refresh() {
        Header::refreshStyle();
        FormWidget::refreshStyle();
    }

} // namespace rckid::ui