#pragma once

#include <rckid/filesystem.h>
#include <rckid/contacts.h>
#include <rckid/ui/app.h>
#include <rckid/ui/panel.h>
#include <rckid/apps/launcher.h>
#include <rckid/apps/dialogs/contact_dialog.h>

namespace rckid {


    class Friends : public ui::Widget {
    public:

        static unique_ptr<LauncherMenu> generateLauncherMenu() {
            auto menu = std::make_unique<LauncherMenu>();
            auto overlay = std::make_unique<Friends>();
            ui::with(overlay.get())
                << ui::SetRect(Rect::XYWH(0, 0, 320, 240));

            Contact::readAll([overlay = overlay.get()](unique_ptr<Contact> c) {
                 overlay->contacts_.push_back(std::move(c));
            });

            for (auto & c : overlay->contacts_) {
                (*menu)
                    << ui::MenuItem{c->name, c->image, [c = c.get(), overlay = overlay.get()]() {
                        // read the contacts again, then take the contact we need and run the dialog for it 
                        auto x = App::run<ContactDialog>(c);
                        if (x.has_value() && x.value())
                            overlay->dirty_ = true;
                    }};
            }

            menu->setOverlay(std::move(overlay));
            return menu;
        }

        ~Friends() override {
            if (dirty_)
                Contact::saveAll(contacts_.begin(), contacts_.end());
        }

        void processEvents() override {
            if (btnPressed(Btn::Select)) {
                ui::Menu popup_;
                popup_ 
                    << ui::MenuItem("New", assets::icons_16::plus, [this]() {
                        auto name = App::run<TextDialog>();
                        if (name) {
                            auto c = std::make_unique<Contact>(name.value());
                            App::run<ContactDialog>(c.get());
                            contacts_.push_back(std::move(c));
                            dirty_ = true;
                            // TOOD trigger menu refresh by requiring resource clear
                            // or have thelauncher menu generator support menu refresh after action taken
                            // but need to figure how
                        }
                    });
                if (!contacts_.empty())
                    popup_
                        << ui::MenuItem("Delete", assets::icons_16::remove, [this](){
                            /*
                            // TODO add confirmation dialog
                            // delete the contact (and its menu item)
                            uint32_t idx = carousel_->index();
                            contacts_.erase(contacts_.begin() + idx);
                            carousel_->menu()->erase(carousel_->menu()->begin() + idx);
                            if (contacts_.empty()) {
                                carousel_->setEmpty();
                            } else {
                                if (idx >= contacts_.size())
                                  --idx;
                                carousel_->setItem(idx);
                            }
                            // and flag as dirty
                            dirty_ = true;
                            */
                        });
                auto action = App::run<PopupMenu>(popup_);
                if (action)
                    action.value()->action()();
            }

        }

    private:
        std::vector<std::unique_ptr<Contact>> contacts_;
        bool dirty_ = false;

    }; // rckid::Friends

#ifdef HAHA

    class Friends : public ui::App<void> {
    public:
        String name() const override { return "Friends"; }

        Friends() {
            using namespace ui;
            carousel_ = addChild(new Launcher::B_orrowedCarousel());
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
                                auto x = App::run<ContactDialog>(contact);
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
            ui::App<void>::onBlur();
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
                exit();
            }
            if (btnPressed(Btn::Select)) {
                ui::Menu popup_;
                popup_ 
                    << ui::MenuItem("New", assets::icons_16::plus, [this]() {
                        auto name = App::run<TextDialog>();
                        if (name) {
                            Contact * c = new Contact{name.value()};
                            dirty_ = true;
                            // where to add the contact?
                            uint32_t idx = 0;
                            for (uint32_t e = contacts_.size(); idx < e; ++idx) {
                                if (contacts_[idx]->name >= c->name)
                                    break;
                            }
                            // insert the contact and create appropriate menu item
                            contacts_.insert(contacts_.begin() + idx, unique_ptr<Contact>{c});
                            carousel_->menu()->insert(
                                carousel_->menu()->begin() + idx, 
                                ui::MenuItem{c->name, c->image, [this, c]() {
                                    auto x = App::run<ContactDialog>(c);
                                    dirty_ = dirty_ || (x.has_value() && x.value());
                                }}
                            );
                            carousel_->setItem(idx);
                        }
                    });
                if (!carousel_->empty())
                    popup_
                        << ui::MenuItem("Delete", assets::icons_16::remove, [this](){
                            // TODO add confirmation dialog
                            // delete the contact (and its menu item)
                            uint32_t idx = carousel_->index();
                            contacts_.erase(contacts_.begin() + idx);
                            carousel_->menu()->erase(carousel_->menu()->begin() + idx);
                            if (contacts_.empty()) {
                                carousel_->setEmpty();
                            } else {
                                if (idx >= contacts_.size())
                                  --idx;
                                carousel_->setItem(idx);
                            }
                            // and flag as dirty
                            dirty_ = true;
                        });
                auto action = App::run<PopupMenu>(popup_);
                if (action)
                    action.value()->action()();
            }
        }

    private:
        Launcher::BorrowedCarousel * carousel_;
        std::vector<std::unique_ptr<Contact>> contacts_;
        bool dirty_ = false;
        bool launch_ = false;



    }; // rckid::Friends

    #endif
} // namespace rckid