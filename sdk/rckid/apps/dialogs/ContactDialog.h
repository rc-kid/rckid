#pragma once

#include "../../app.h"
#include "../../pim.h"
#include "../../ui/header.h"
#include "../../ui/carousel.h"
#include "../../utils/json.h"
#include "../../assets/fonts/OpenDyslexic64.h"

namespace rckid {

    /** Contact dialog allows selecting contact from those stored on the device. 
     */
    class ContactDialog : public ui::Form<Contact> {
    public:

        String name() const override { return "ContactDialog"; }

        ContactDialog() : 
            ui::Form<Contact>{320, 240},
            c_{
                [this](){ return contacts_.size(); },
                [this](uint32_t index, Direction direction) {
                    Contact const & c = contacts_[index];
                    c_.set(c.name, Icon{c.image}, direction);
                }
            } {
            g_.addChild(c_);
            c_.setRect(Rect::XYWH(0, 160, 320, 80));
            c_.setFont(Font::fromROM<assets::OpenDyslexic64>());
        }

        void update() override {
            ui::Form<Contact>::update();
            if (btnPressed(Btn::Down) || btnPressed(Btn::B)) {
                btnClear(Btn::Down);
                btnClear(Btn::B);
                return exit();
            }
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                btnClear(Btn::A);
                btnClear(Btn::Up);
                if (contacts_.size() == 0)
                    return;
                Contact const & c = contacts_[c_.currentIndex()];
                return exit(c);
            }
        }

        void focus() override {
            ui::Form<Contact>::focus();
            c_.focus();
            // if we have contact
            loadContacts();
            if (firstRun_) {
                if (contacts_.size() > 0)
                    c_.setItem(0, Direction::Up);
                else
                    c_.showEmpty(Direction::Up);
                firstRun_ = false;
            } else if (contacts_.size() > c_.currentIndex())
                c_.setItem(c_.currentIndex());
            else if (contacts_.size() > 0)
                c_.setItem(0);
            else
                c_.showEmpty();
        }

        void blur() {
            // cleanup for the next app
            contacts_.clear();
        }

        Point iconPosition() const { return c_.iconPosition(); }
        Point textPosition() const { return c_.textPosition(); }

    private:

        static constexpr char const * CONTACTS_PATH = "/contacts.ini";

        void loadContacts() {
            contacts_.clear();
            Contact::forEach([this](Contact c) {
                contacts_.push_back(std::move(c));
            });
            if (contacts_.size() > 0) {
                LOG(LL_INFO, "Loaded " << (uint32_t) contacts_.size() << " contacts");
            } else {
                LOG(LL_INFO, "No contacts file found, or empty");
            }
        }

        ui::EventBasedCarousel c_;

        std::vector<Contact> contacts_;
        // determines on focus if this has been the first time the dialog appears (different transition is used then)
        bool firstRun_ = true;
    }; // rckid::ContactDialog


} // namespace rckid