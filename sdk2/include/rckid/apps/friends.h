#pragma once

#include <rckid/filesystem.h>
#include <rckid/contacts.h>
#include <rckid/ui/app.h>
#include <rckid/apps/launcher.h>
#include <rckid/apps/dialogs/contact_dialog.h>

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
                        (*menu)
                            << ui::MenuItem{contact->name, contact->image, [this, contact = contact.get()]() {
                                auto x = ContactDialog{contact}.run();
                                dirty_ = dirty_ || (x.has_value() && x.value());
                            }};
                    }
                    return menu;
                });
        }

        void onFocus() override {
            ui::App<void>::onFocus();
            focusWidget(carousel_);
            if (!launch_)
                return;
            carousel_->moveDown();
        }

        void onBlur() override {
            if (!launch_)
                return;
            carousel_->moveUp(nullptr);
            waitUntilIdle(carousel_);
        }

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                auto action = carousel_->currentItem()->action();
                launch_ = true;
                action();
                launch_ = false;
            }
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
        bool launch_ = false;



    }; // rckid::Friends
} // namespace rckid