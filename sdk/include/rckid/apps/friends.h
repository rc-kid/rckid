#pragma once

#include <rckid/filesystem.h>
#include <rckid/contacts.h>
#include <rckid/ui/app.h>
#include <rckid/ui/panel.h>
#include <rckid/apps/launcher.h>
#include <rckid/apps/dialogs/contact_dialog.h>

namespace rckid {


    class Friends : public LauncherOverlay {
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
                            // saves the contacts so that the menu refresh will reload the contacts without the deleted one
                            Contact::saveAll(contacts_.begin(), contacts_.end());
                            // trigger menu refresh
                            ASSERT(launcher() != nullptr);
                            launcher()->refreshMenu();
                        }
                    });
                if (!contacts_.empty())
                    popup_
                        << ui::MenuItem("Delete", assets::icons_16::remove, [this](){
                            // TODO add confirmation dialog
                            // delete the contact (and its menu item)
                            ASSERT(launcher() != nullptr);
                            uint32_t idx = launcher()->currentIndex();
                            contacts_.erase(contacts_.begin() + idx);
                            // saves the contacts so that the menu refresh will reload the contacts without the deleted one
                            Contact::saveAll(contacts_.begin(), contacts_.end());
                            
                            launcher()->refreshMenu();
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

} // namespace rckid