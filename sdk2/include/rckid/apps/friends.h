#pragma once

#include <rckid/filesystem.h>
#include <rckid/contacts.h>
#include <rckid/ui/app.h>
#include <rckid/apps/launcher.h>

namespace rckid {

    class Friends : public ui::App<void> {
    public:
        String name() const override { return "Friends"; }

        Friends() {
            using namespace ui;
            carousel_ = addChild(new Launcher::BorrowedCarousel());
        }

        ~Friends() override {
            if (dirty_)
                Contact::saveAll(contacts_.begin(), contacts_.end());
        }

    protected:

        void onLoopStart() override {
            using namespace ui;
            Contact::readAll([this](unique_ptr<Contact> c) {
                 contacts_.push_back(std::move(c));
            });
            with(carousel_)
                << ResetMenu([this]() { 
                    auto menu = std::make_unique<ui::Menu>();
                    for (auto & contact : contacts_) {
                        menu->emplace_back(contact->name, contact->image, [contact = contact.get()]() {
                            // TODO show contact details
                        });
                    }
                    return menu;
                });
        }

        void onFocus() override {
            ui::App<void>::onFocus();
            focusWidget(carousel_);
        }

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                ASSERT(carousel_->atRoot());
                // TODO terminate music, etc
                exit();
            }
        }

    private:
        Launcher::BorrowedCarousel * carousel_;
        std::vector<std::unique_ptr<Contact>> contacts_;
        bool dirty_ = false;



    }; // rckid::Friends
} // namespace rckid