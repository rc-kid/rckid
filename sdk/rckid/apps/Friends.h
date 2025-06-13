#pragma once

#include "dialogs/ContactDialog.h"

namespace rckid {

    /** The friends app is a simple ContactDialog in a launcher mode where the launched app is contact viewer & editor. 
     */
    class Friends : public App {
    public:
        /** Signle Contact Viewer and editor. 
         */
        class ContactViewer : public ui::App<void> {
        public:




        private:
            Contact c_;
            ui::Image * image_ = nullptr;
            ui::Label * name_ = nullptr;
            ui::Label * birthday_ = nullptr;
            ui::Label * id_ = nullptr;
            ui::Label * email_ = nullptr;
            ui::Label * phone_ = nullptr;
            ui::Label * address_ = nullptr;
            ui::Label * note_ = nullptr;
        }; // ContactViewer

        Friends() : App{} {}

    protected:

        friend class App;

        void draw() override {}

        void loop() override {
            ContactDialog * cd = new ContactDialog{};
            // run the contact dialog in launcher mode with contact viewer as callback app
            cd->run([](Contact c) {
                // TODO
            });
        }
    }; // rckid::Friends
} // namespace rckid