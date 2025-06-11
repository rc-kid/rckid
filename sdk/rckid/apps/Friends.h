#pragma once

#include "../app.h"
#include "../pim.h"
#include "../ui/header.h"
#include "../ui/carousel.h"
#include "../utils/json.h"
#include "../assets/fonts/OpenDyslexic64.h"

namespace rckid {


    /** A simple app that manages contacts & their birthdays. 

        It allows adding new friends, viewing their birthdays, etc. 
     
     */
    class Friends : public ui::App<void> {
    public:

        Friends() : ui::App<void>{320, 240} {
            c_ = g_.addChild(new ui::Carousel{});
            c_->setRect(Rect::XYWH(0, 160, 320, 80));
            c_->setFont(Font::fromROM<assets::OpenDyslexic64>());
            loadContacts();
            if (contacts_.size() > 0)
                setContact(0);
        }

        void update() override {
            ui::App<void>::update();
            if (btnPressed(Btn::Down) || btnPressed(Btn::B)) {
                btnClear(Btn::Down);
                btnClear(Btn::B);
                return exit();
            }
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

        void setContact(uint32_t i, Direction transition = Direction::Up) {
            ASSERT(i < contacts_.size());
            Contact const & c = contacts_[i];
            // load the icon associated with the contact
            NewArenaGuard g{};
            auto f = fs::fileRead(STR("/images/" << c.image));
            // tell the carousel
            c_->set(c.name, Bitmap<ColorRGB>{ARENA(PNG::fromStream(f))}, transition);
            // and store the index
            i_ = i;
        }

        ui::Carousel * c_;

        std::vector<Contact> contacts_;
        uint32_t i_ = 0;

    }; // rckid::Friends

} // namespace rckid