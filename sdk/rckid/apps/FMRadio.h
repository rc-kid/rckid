#pragma once

#include "../app.h"
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
        
        String name() const override { return "FMRadio"; }

        FMRadio() :
            ui::Form<void>{},
            freq_{Rect::XYWH(0, 20, 320, 128), "???"},
            stationName_{Rect::XYWH(0, 130, 320, 32), ""},
            radioText1_{Rect::XYWH(0, 160, 320, 32), ""},
            radioText2_{Rect::XYWH(0, 190, 320, 32), ""},
            signal_{Rect::XYWH(0, 25, 320, 20), "56 78 STEREO"}
        {
            freq_.setFont(Font::fromROM<assets::OpenDyslexic128>());
            freq_.setHAlign(HAlign::Center);
            freq_.setVAlign(VAlign::Center);
            stationName_.setFont(Font::fromROM<assets::OpenDyslexic32>());
            radioText1_.setFont(Font::fromROM<assets::OpenDyslexic32>());
            radioText2_.setFont(Font::fromROM<assets::OpenDyslexic32>());
            g_.addChild(freq_);
            g_.addChild(stationName_);
            g_.addChild(radioText1_);
            g_.addChild(radioText2_);
            g_.addChild(signal_);
            radio_ = Radio::instance();
            if (radio_ != nullptr) {
                LOG(LL_INFO, "Si4705 info:");
                radio_->enable(true);
                cpu::delayMs(1000);
                auto version = radio_->getVersionInfo();
                LOG(LL_INFO, "  part #:        " << version.partNumber);
                LOG(LL_INFO, "  fw:            " << version.fwMajor << "." << version.fwMinor);
                LOG(LL_INFO, "  patch:         " << version.patch);
                LOG(LL_INFO, "  comp:          " << version.compMajor << "." << version.compMinor);
                LOG(LL_INFO, "  chip revision: " << version.chipRevision);
                LOG(LL_INFO, "  cid:           " << version.cid);
                radio_->enableGPO1(true);
                radio_->setFrequency(9370);
            }
        }

        ~FMRadio() override {
            if (radio_ != nullptr) {
                radio_->enable(false);
            }
        }

    protected:


        void update() override {
            ui::Form<void>::update();
            if (radio_ == nullptr) {
                InfoDialog::error("No radio", "This device does not have a radio chip.");
                exit();
            }
            refresh_ = radio_->update();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                exit();
            }
            if (btnPressed(Btn::Left))
                //radio_->enableEmbeddedAntenna(true);
                radio_->seekDown();
            if (btnPressed(Btn::Up)) {

                //radio_->enableGPO1(true);
                //radio_->setGPO1(true);
            }
            if (btnPressed(Btn::Right)) {
                //radio_->enableEmbeddedAntenna(false);
                radio_->seekUp();
                //radio_->enableGPO1(true);
                //radio_->setGPO1(false);
            }
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
        Radio * radio_;
        bool refresh_ = false;
        ui::Label freq_;
        ui::Label stationName_;
        ui::Label radioText1_;
        ui::Label radioText2_;
        ui::Label signal_;

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
                return "STEREO";
            else if (stereo >= 50)
                return "BLENDED";
            else
                return "MONO"; 
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

    }; // rckid::Radio




} // namespace rckid