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
            c_ = g_.addChild(new ui::Carousel{});
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
            if (! contacts_.empty()) {
                if (btnDown(Btn::Left) && c_->idle()) {
                    if (i_ > 0) {
                        setContact(i_ - 1, Direction::Left);
                    } else {
                        setContact(contacts_.size() - 1, Direction::Left);
                    }
                }
                if (btnDown(Btn::Right) && c_->idle()) {
                    if (i_ + 1 < contacts_.size()) {
                        setContact(i_ + 1, Direction::Right);
                    } else {
                        setContact(0, Direction::Right);
                    }
                }
            }
        }

        void focus() override {
            ui::App<Contact>::focus();
            // if we have contact
            loadContacts();
            if (i_ == FIRST_RUN) {
                if (contacts_.size() > 0)
                    setContact(0, Direction::Up);
                else
                    c_->showEmpty(Direction::Up);
            } else if (contacts_.size() > i_)
                setContact(i_);
            else if (contacts_.size() > 0)
                setContact(0);
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

        void setContact(uint32_t i, Direction transition = Direction::None) {
            ASSERT(i < contacts_.size());
            Contact const & c = contacts_[i];
            // load the icon associated with the contact
            NewArenaGuard g{};
            auto f = fs::fileRead(c.image);
            // tell the carousel
            if (f.good()) 
                c_->set(c.name, Bitmap<ColorRGB>{ARENA(PNG::fromStream(f))}, transition);
            else
                c_->set(c.name, Bitmap<ColorRGB>{ARENA(PNG::fromBuffer(assets::icons_64::girl))}, transition);
            // and store the index
            i_ = i;
        }

        static constexpr uint32_t FIRST_RUN = 0xffffffff;

        ui::Carousel * c_;

        std::vector<Contact> contacts_;
        uint32_t i_ = FIRST_RUN; // first run
    }; // rckid::ContactDialog


} // namespace rckid