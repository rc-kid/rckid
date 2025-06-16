#pragma once

#include "../../app.h"
#include "../../pim.h"
#include "../../ui/header.h"
#include "../../ui/carousel.h"
#include "../../utils/json.h"
#include "../../assets/fonts/OpenDyslexic64.h"

namespace rckid {

    class ContactDialog : public ui::App<Contact> {
    public:

        ContactDialog() : ui::App<Contact>{320, 240} {
            c_ = g_.addChild(new ui::EventBasedCarousel{
                [this](){ return contacts_.size(); },
                [this](uint32_t index, Direction direction) {
                    Contact const & c = contacts_[index];
                    c_->set(c.name, Icon{c.image}, direction);
                }
            });
            c_->setRect(Rect::XYWH(0, 160, 320, 80));
            c_->setFont(Font::fromROM<assets::OpenDyslexic64>());
        }

        void update() override {
            ui::App<Contact>::update();
            if (btnPressed(Btn::Down) || btnPressed(Btn::B)) {
                btnClear(Btn::Down);
                btnClear(Btn::B);
                return exit();
            }
            c_->processEvents();
        }

        void focus() override {
            ui::App<Contact>::focus();
            // if we have contact
            loadContacts();
            if (firstRun_) {
                if (contacts_.size() > 0)
                    c_->setItem(0, Direction::Up);
                else
                    c_->showEmpty(Direction::Up);
                firstRun_ = false;
            } else if (contacts_.size() > c_->currentIndex())
                c_->setItem(c_->currentIndex());
            else if (contacts_.size() > 0)
                c_->setItem(0);
            else
                c_->showEmpty();
        }

        void blur() {
            // cleanup for the next app
            contacts_.clear();
        }

    private:

        static constexpr char const * CONTACTS_PATH = "/contacts.json";

        void loadContacts() {
            fs::mount(fs::Drive::SD);
            contacts_.clear();
            if (fs::exists(CONTACTS_PATH)) {
                fs::FileRead f = fs::fileRead(CONTACTS_PATH);
                json::Object contacts = json::parse(f);
                if (! contacts.isArray()) {
                    LOG(LL_ERROR, "Contacts file is not an array");
                    return;
                }
                for (auto const & item : contacts)
                    contacts_.push_back((Contact{item}));
                LOG(LL_INFO, "Loaded " << (uint32_t) contacts_.size() << " contacts");
            } else {
                LOG(LL_INFO, "No contacts file found, starting with an empty list");
            }
        }

        ui::EventBasedCarousel * c_;

        std::vector<Contact> contacts_;
        // determines on focus if this has been the first time the dialog appears (different transition is used then)
        bool firstRun_ = true;
    }; // rckid::ContactDialog


} // namespace rckid