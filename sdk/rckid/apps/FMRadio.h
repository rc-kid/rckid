#pragma once

#include "../app.h"
#include "../ui/form.h"
#include "../radio.h"
#include "../ui/label.h"
#include "../assets/fonts/OpenDyslexic128.h"

#include "dialogs/InfoDialog.h"

namespace rckid {

    /** A simple FM radio. 
     */
    class FMRadio : public ui::App<void> {
    public:
        FMRadio() :
            ui::App<void>{},
            freq_{Rect::XYWH(0, 30, 320, 130), ""},
            rds_{Rect::XYWH(0, 170, 320, 80), ""} {
            freq_.setFont(Font::fromROM<assets::OpenDyslexic128>());
            freq_.setHAlign(HAlign::Center);
            freq_.setVAlign(VAlign::Center);
            g_.addChild(freq_);
            g_.addChild(rds_);
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
                cpu::delayMs(100);
                auto tuneStatus = radio_->getTuneStatus();
                LOG(LL_INFO, "  frequency:     " << tuneStatus.frequency10kHz() << " [10kHz]");
                LOG(LL_INFO, "  rssi:          " << tuneStatus.rssi());
                LOG(LL_INFO, "  snr:           " << tuneStatus.snr());
                LOG(LL_INFO, "  multipath:     " << tuneStatus.multipath());
                LOG(LL_INFO, "  antCap:        " << tuneStatus.antCap());
            }
        }

    protected:

        void update() override {
            ui::App<void>::update();
            if (radio_ == nullptr) {
                InfoDialog::error("No radio", "This device does not have a radio chip.");
                exit();
            }
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                exit();
            }
            if (btnPressed(Btn::Left))
                radio_->enableEmbeddedAntenna(true);
                //radio_->seekUp();
            if (btnPressed(Btn::Up)) {
                auto rsq = radio_->getRSQStatus();
                LOG(LL_INFO, "  valid:         " << rsq.valid());
                LOG(LL_INFO, "  afcRail:       " << rsq.afcRail());
                LOG(LL_INFO, "  softMute:      " << rsq.softMute());
                LOG(LL_INFO, "  stereoPilot:   " << rsq.stereoPilot());
                LOG(LL_INFO, "  stereo:        " << rsq.stereo());
                LOG(LL_INFO, "  rssi:          " << rsq.rssi());
                LOG(LL_INFO, "  snr:           " << rsq.snr());
                LOG(LL_INFO, "  multipath:     " << rsq.multipath());
                LOG(LL_INFO, "  freqOffset:    " << rsq.frequencyOffset());

                //radio_->enableGPO1(true);
                //radio_->setGPO1(true);
            }
            if (btnPressed(Btn::Right)) {
                radio_->enableEmbeddedAntenna(false);
                //radio_->seekDown();
                //radio_->enableGPO1(true);
                //radio_->setGPO1(false);
            }
        }

        void draw() override {
            rds_.setText(STR(hex(radio_->status_.rawResponse()) << " ")); //  << gpio::read(RP_PIN_RADIO_INT)));
            ui::App<void>::draw();
        }

        void blur() {
            if (radio_ != nullptr) {
                radio_->enable(false);
            }
        }

    protected:
        Radio * radio_;
        ui::Label freq_;
        ui::Label rds_;

    }; // rckid::Radio

} // namespace rckid