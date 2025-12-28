#pragma once

#include "../../app.h"
#include "../../filesystem.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../ui/image.h"
#include "../../assets/icons_64.h"
#include "../../assets/fonts/OpenDyslexic64.h"
#include "../../audio/audio.h"
#include "../../utils/ini.h"
#include "../../pim.h"

namespace rckid {

    /** Alarm 
     */
    class Alarm : public ui::Form<void> {
    public:

        String name() const override { return "Alarm"; }

        String title() const override { return ""; }

        class Settings {
        public:

            String const & sound() const { return sound_; }

            Settings & setSound(String const & s) { sound_ = s; return *this; }

            TinyTime silentStart() const { return silentStart_; }
            TinyTime silentEnd() const { return silentEnd_; }

            Settings & setSilentPeriod(TinyTime start, TinyTime end) {
                silentStart_ = start;
                silentEnd_ = end;
                return *this;
            }

            bool isSilentPeriod(TinyTime const & t) const {
                // if start is less than end, then we have single period from start to end
                if (silentStart_ < silentEnd_)
                    return (t >= silentStart_) && (t < silentEnd_);
                else 
                    return (t >= silentStart_) || (t < silentEnd_);
            }

            static Settings load() {
                ini::Reader reader{fs::fileRead("/alarm.ini")};
                Settings result{};
                while (auto section = reader.nextSection()) {
                    if (section.value() == "Alarm") {
                        while (auto kv = reader.nextValue()) {
                            if (kv->first == "silentStart") {
                                result.silentStart_.setFromString(kv->second.c_str());
                            } else if (kv->first == "silentEnd") {
                                result.silentEnd_.setFromString(kv->second.c_str());
                            } else if (kv->first == "sound") {
                                result.sound_ = kv->second;
                            }
                        }
                    }
                }    
                return result;
            }

            void save() {
                ini::Writer writer{fs::fileWrite("/alarm.ini")};
                writer.writeSection("Alarm");
                writer.writeValue("silentStart", STR(silentStart_.hour() << ":" << silentStart_.minute() << ":00"));
                writer.writeValue("silentEnd", STR(silentEnd_.hour() << ":" << silentEnd_.minute() << ":00"));
                writer.writeValue("sound", sound_);
            }

        private:
            TinyTime silentStart_{20, 0, 0};
            TinyTime silentEnd_{ 7, 0, 0};
            String sound_{"/files/music/alarm.mp3"};
        }; // Alarm::Settings

        class Event {
        public:
            String text = "WakeUp";
            Icon icon{assets::icons_64::alarm_clock};
            String sound = "/files/music/alarm.mp3";

            Event(String text, Icon icon, String sound):
                text{std::move(text)}, icon{icon}, sound{std::move(sound)} {
            }

            Event(ini::Reader & reader) {
                while (auto kv = reader.nextValue()) {
                    if (kv->first == "text") {
                        text = kv->second;
                    } else if (kv->first == "icon") {
                        icon = Icon{kv->second.c_str()};
                    } else if (kv->first == "sound") {
                        sound = kv->second;
                    }
                }
            }

            void save(ini::Writer & writer) {
                writer.writeSection("Event");
                writer.writeValue("text", text);
                if (icon.isFile())
                    writer.writeValue("icon", icon.filename());
                writer.writeValue("sound", sound);
            }
        }; 

        Alarm(String title, Icon icon, String music):
            ui::Form<void>{},
            title_{title}
        {
            active_ |= ALARM_ACTIVE;
            icon_ = g_.addChild(new ui::Image{icon});
            status_ = g_.addChild(new ui::Label{Rect::XYWH(0, 130, 320, 64), title_});
            status_->setFont(Font::fromROM<assets::OpenDyslexic64>());
            icon_->setTransparent(true);
            icon_->setPos(160 - icon_->width() / 2, 60);
            status_->setHAlign(HAlign::Center);
            if (! music.empty() && ! audioPlayback()) {
                as_ = AudioStream::fromFile(music);
                if (as_)
                    audioPlay(*as_);
            }
        }

        Alarm(Event event): Alarm{std::move(event.text), event.icon, std::move(event.sound)} {}

        ~Alarm() {
            audioStop();
            delete as_;
            active_ &= ~ALARM_ACTIVE;
        }

        static void addSpecialEvent(Event event) {
            ini::Writer writer{fs::fileAppend("/alarm-event.ini")};
            event.save(writer);
        }

        static void checkAlarm() {
            if (active_ & ALARM_ACTIVE)
                return;
            // if there is active alarm, it has the highest priority, get the audio alarm sound
            if (alarm()) {
                App::run<Alarm>("Wake Up!", Icon{assets::icons_64::alarm_clock}, Settings::load().sound());
            }
        }

        static void checkEvent() {
            if (active_ & ALARM_ACTIVE)
                return;
            checkAlarm();
            // if there is special event, show that now
            {
                ini::Reader reader{fs::fileRead("/alarm-event.ini")};
                while (auto section = reader.nextSection()) {
                    if (section.value() == "Event") {
                        Event event{reader};
                        App::run<Alarm>(event);
                    }
                }
                // remove the event file
                fs::eraseFile("/alarm-event.ini");
            }
            // play happy bday *if* its our bday and if birthday event is defined
            if (fs::isFile("/alarm-birthday.ini")) {
                if (timeNow().date.isAnnualEqual(Myself::contact().birthday)) {
                    ini::Reader reader{fs::fileRead("/alarm-birthday.ini")};
                    while (auto section = reader.nextSection()) {
                        if (section.value() == "Event") {
                            Event event{reader};
                            App::run<Alarm>(event);
                        }
                    }
                }
            }
        }

    protected:

        void update() override {
            ui::Form<void>::update();
            if (as_ != nullptr)
                as_->update();
            if (++ticks_ % 30 == 0)
                rumblerEffect(RumblerEffect::OK());
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();
        }

    private:
        String title_;
        ui::Image * icon_;
        ui::Label * status_;
        AudioStream * as_ = nullptr;
        uint32_t ticks_ = 0;

        static constexpr uint32_t ALARM_ACTIVE = 1 << 0;
        static inline uint32_t active_ = 0;

    }; // Alarm
} // namespace rckid