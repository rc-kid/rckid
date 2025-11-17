#pragma once

#include "../app.h"
#include "../task.h"
#include "../ui/form.h"
#include "../radio.h"
#include "../ui/label.h"
#include "../assets/fonts/OpenDyslexic128.h"

#include "dialogs/InfoDialog.h"

namespace rckid {

    /** A simple FM radio. 
     
        TODO
        - allow switching between mono/stereo
        - allow switching internal/headphone antenna
        - allow forcing speaker even if headphones are on
        - allow playing music while the app is not active
        - presets

     */
    class FMRadio : public ui::Form<void> {
    public:
        
        String name() const override { return "Radio"; }

        FMRadio() :
            ui::Form<void>{},
            freq_{Rect::XYWH(0, 20, 320, 128), "---"},
            stationName_{Rect::XYWH(0, 130, 320, 32), "Initializing..."},
            radioText1_{Rect::XYWH(0, 160, 320, 32), ""},
            radioText2_{Rect::XYWH(0, 190, 320, 32), ""},
            signal_{Rect::XYWH(0, 25, 320, 20), "-- -- -- ----"}
        {
            freq_.setFont(Font::fromROM<assets::OpenDyslexic128>());
            freq_.setHAlign(HAlign::Center);
            freq_.setVAlign(VAlign::Center);
            stationName_.setFont(Font::fromROM<assets::OpenDyslexic32>());
            radioText1_.setFont(Font::fromROM<assets::OpenDyslexic32>());
            radioText2_.setFont(Font::fromROM<assets::OpenDyslexic32>());
            stationName_.setColor(ui::Style::accentFg());
            radioText1_.setColor(ui::Style::accentFg());
            radioText2_.setColor(ui::Style::accentFg());
            signal_.setColor(ui::Style::accentFg());
            g_.addChild(freq_);
            g_.addChild(stationName_);
            g_.addChild(radioText1_);
            g_.addChild(radioText2_);
            g_.addChild(signal_);
            radio_ = Radio::instance();
            if (radio_ != nullptr) {
                // enable the embedded antenna if we do not have headphones attached
                radio_->enableEmbeddedAntenna(! audioHeadphones());
                // ensure the background task is running
                RadioTask::instance();
                initDelay_ = 60; // wait one second before loading settings & setting frequency
                contextMenu_.add(ui::ActionMenu::Item("Presets", [this](){
                    auto p = App::run<PresetMenu>(this);
                    if (p.has_value())
                        radio_->setFrequency(p.value().frequency);
                }));
                contextMenu_.add(ui::ActionMenu::Item("Add preset", [this](){
                    auto name = App::run<TextDialog>("Preset name", radio_->stationName());
                    if (name.has_value())
                        presets_.push_back(Preset{name.value(), radio_->frequency()});
                }));
            }
        }

        ~FMRadio() override {
            RadioTask::deleteInstance();
            saveSettings();
        }

    protected:

        void loadSettings() {
            bool fSet = false;
            if (fs::isMounted()) {
                String path = fs::join(homeFolder(), "settings.ini");
                if (fs::exists(path)) {
                    ini::Reader ini{fs::fileRead(fs::join(homeFolder(), "settings.ini"))};
                    while (auto section = ini.nextSection()) {
                        if (section.value() == "settings") {
                            while (auto kv = ini.nextValue()) {
                                if (kv->first == "frequency") {
                                    uint32_t freq = std::atoi(kv->second.c_str());
                                    radio_->setFrequency(freq);
                                    fSet = true;
                                } else {
                                    LOG(LL_ERROR, "Unknown FM radio setting: " << kv->first);
                                }
                            }
                        } else if (section.value() == "preset") {
                            Preset preset;
                            while (auto kv = ini.nextValue()) {
                                if (kv->first == "frequency") {
                                    preset.frequency = std::atoi(kv->second.c_str());
                                } else if (kv->first == "name") {
                                    preset.name = kv->second;
                                } else {
                                    LOG(LL_ERROR, "Unknown FM radio preset setting: " << kv->first);
                                }
                            }
                            presets_.push_back(preset);
                        } else {
                            LOG(LL_ERROR, "Invalid settings section: " << section.value());
                        }
                    }
                }
            }
            if (! fSet)
                radio_->setFrequency(9370);
        }

        void update() override {
            ui::Form<void>::update();
            if (initDelay_ > 0 && --initDelay_ == 0)
                loadSettings();
            if (audioHeadphonesChanged())
                radio_->enableEmbeddedAntenna(! audioHeadphones());
            if (radio_ == nullptr) {
                InfoDialog::error("No radio", "This device does not have a radio chip.");
                exit();
            }
            refresh_ = radio_->update();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                exit();
            }
            // left and right to seek directly
            if (btnPressed(Btn::Left))
                radio_->seekDown();
            if (btnPressed(Btn::Right))
                radio_->seekUp();
            if (btnPressed(Btn::Select)) {
                auto action = App::run<PopupMenu<ui::Action>>(&contextMenu_);
                if (action.has_value())
                    action.value()();
            }
            /*
            if (btnPressed(Btn::Select))
                radio_->enableEmbeddedAntenna(false);
            if (btnPressed(Btn::Start))
                radio_->enableEmbeddedAntenna(true);
            */
        }

        void draw() override {
            if (refresh_) {
                refresh_ = false;
                refreshUi();
            }
            //rds_.setText(STR(hex(radio_->status_.rawResponse()) << " ")); //  << gpio::read(RP_PIN_RADIO_INT)));
            ui::Form<void>::draw();
        }

    private:

        class Preset {
        public:
            String name;
            uint16_t frequency;

            Preset() = default;

            Preset(String name, uint16_t frequency) :
                name{std::move(name)},
                frequency{frequency} {
            }
        }; // FMRadio::Preset

        /** Background task for radio player. 
         
            Simply keeps the radio enabled even if the app is not running
         */
        class RadioTask : public Task {
        public:

            static RadioTask * instance() {
                if (instance_ == nullptr)
                    instance_ = new RadioTask{};
                return instance_;
            }

            static void deleteInstance() {
                delete instance_;
                instance_ = nullptr;
            }

            Radio * radio() const { return radio_;}

        protected:

            RadioTask():
                radio_{Radio::instance()} 
            {
                ASSERT(instance_ == nullptr);
                if (radio_ != nullptr) {
                    radio_->enable(true);
                    radio_->enableEmbeddedAntenna(! audioHeadphones());
                }
            }

            ~RadioTask() override {
                instance_ = nullptr;
                if (radio_ != nullptr)
                    radio_->enable(false);
            }

            /** The run task simply checks whether we have a headphone attach/detach and switches the embedded antenna internally.
             */
            void tick() override {
                if (audioHeadphonesChanged())
                    radio_->enableEmbeddedAntenna(! audioHeadphones());
                keepAlive();
            }

            Radio * radio_;
            uint32_t startup_ = 60; // we need to wait one second before the radio is ready

            static inline RadioTask * instance_ = nullptr;
        }; 

        class PresetMenu : public ui::Form<Preset> {
        public:
        
            String name() const override { return "Radio"; }

            String title() const override { return "Select preset"; }

            PresetMenu(FMRadio * radio):
                ui::Form<Preset>{Rect::XYWH(0, 144, 320, 96), /* raw */ true},
                radio_{radio},
                c_{
                    [this]() {
                        return radio_->presets_.size();
                    },
                    [this](uint32_t index, Direction direction) {
                        Preset & p = radio_->presets_[index];
                        c_.set(p.name, Icon{assets::icons_64::music_wave}, direction);
                    }
                }
            { 
                contextMenu_.add(ui::ActionMenu::Item("Rename", [this]() {
                    if (radio_->presets_.size() == 0)
                        return;
                    auto name = App::run<TextDialog>("Preset name", radio_->presets_[c_.currentIndex()].name);
                    if (name.has_value()) {
                        radio_->presets_[c_.currentIndex()].name = name.value();
                        c_.setItem(c_.currentIndex(), Direction::Up);
                    }
                }));
                contextMenu_.add(ui::ActionMenu::Item("Delete", [this]() {
                    if (radio_->presets_.size() == 0)
                        return;
                    radio_->presets_.erase(radio_->presets_.begin() + c_.currentIndex());
                    c_.setItem(0, Direction::Up);
                }));
                g_.addChild(c_);
                c_.setRect(Rect::XYWH(0, 0, 320, 96));
                c_.setFont(Font::fromROM<assets::OpenDyslexic64>());
                c_.setItem(0, Direction::Up);
            }

        protected:

            void update() override {
                if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                    exit();
                }
                if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                    btnClear(Btn::A);
                    btnClear(Btn::Up);
                    Preset & p = radio_->presets_[c_.currentIndex()];
                    exit(p);
                }
                if (btnPressed(Btn::Select)) {
                    auto action = App::run<PopupMenu<ui::Action>>(&contextMenu_);
                    if (action.has_value())
                        action.value()();
                }
                c_.processEvents();
            }

        private:

            FMRadio * radio_;
            ui::EventBasedCarousel c_;
            ui::ActionMenu contextMenu_;

        }; 

        friend class PresetMenu;

        Radio * radio_;
        bool refresh_ = false;
        ui::Label freq_;
        ui::Label stationName_;
        ui::Label radioText1_;
        ui::Label radioText2_;
        ui::Label signal_;
        ui::ActionMenu contextMenu_;

        std::vector<Preset> presets_;

        uint32_t initDelay_;

        uint32_t irqs_ = 0;

        char const * snrToQuality() {
            uint8_t snr = radio_->snr();
            if (snr >= 25)
                return ">> >> >>";
            else if (snr >= 20)
                return ">> >> --";
            else if (snr >= 15)
                return ">> -- --";
            else
                return "-- -- --";
        }

        char const * stereoToStr() {
            uint8_t stereo = radio_->stereo();
            if (stereo >= 80)
                return "STEREO ";
            else if (stereo >= 50)
                return "BLENDED";
            else
                return " MONO  "; 
        }

        void refreshUi() {
            ASSERT(radio_ != nullptr);
            ++irqs_;
            uint32_t freqMhz = radio_->frequency() / 100;
            uint32_t freqFrac = radio_->frequency() % 100;
            freq_.setText(STR(freqMhz << '.' << freqFrac));
            stationName_.setText(radio_->stationName());
            radioText1_.setText(radio_->radioText1());
            radioText2_.setText(radio_->radioText2());
            signal_.setText((STR(snrToQuality() << "   " << stereoToStr())));
        }

        void saveSettings() {
            if (fs::isMounted() && fs::createFolders(homeFolder())) {
                ini::Writer ini{fs::fileWrite(fs::join(homeFolder(), "settings.ini"))};
                ini.writeSection("settings");
                ini.writeValue("frequency", radio_->frequency());
                // save presets
                for (auto & p : presets_) {
                    ini.writeSection("preset");
                    ini.writeValue("frequency", p.frequency);
                    ini.writeValue("name", p.name);
                }
            }
        }

    }; // rckid::Radio

} // namespace rckid