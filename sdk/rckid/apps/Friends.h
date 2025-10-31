#pragma once

#include "dialogs/ContactDialog.h"
#include "dialogs/TextDialog.h"
#include "dialogs/DateDialog.h"
#include "../ui/scrollview.h"

namespace rckid {

    /** The friends app is a simple ContactDialog in a launcher mode where the launched app is contact viewer & editor. 
     */
    class Friends : public App {
    public:

/*
        std::vector<Anniversary> static getNext(uint32_t n) {
            std::vector<Anniversary> all;
            Contact::forEach([&](Contact c) {
                all.push_back(Anniversary{c});
            });
            Holiday::forEach([&](Holiday h) {
                all.push_back(Anniversary{h});
            });
            std::sort(all.begin(), all.end(), [](Anniversary const & a, Anniversary const & b) {
                return a.daysTillAnniversary() < b.daysTillAnniversary();
            });
            if (all.size() > n)
                all.resize(n);
            return all;
        }
*/

        String name() const override { return "Friends"; }

        /** Signle Contact Viewer and editor. 
         
            Displays the contact information and allows its eedits. 
         */
        class ContactViewer : public ui::Form<void> {
        public:

            /** Use umbrella name for all friends apps.
             */
            String name() const override { return "Friends"; }

            ContactViewer(Contact & c) : 
                ui::Form<void>{Rect::XYWH(0, 0, 320, 240)},
                c_{c},
                image_{8, 18, Icon{c.image}},
                name_{80, 20, c.name},
                birthday_{80, 0, STR(c.birthday.day() << "/" << c.birthday.month() << "/" << c.birthday.year())},
                bdayExtras_{150, 0, STR("(" << c.daysTillBirthday() << " days)")},
                phone_{80, 30, c.phone},
                email_{80, 60, c.email},
                address_{80, 90, c.address},
                note_{80, 120, c.note}
            {
                g_.addChild(image_);
                g_.addChild(name_);
                g_.addChild(contents_);
                contents_.addChild(bdayImg_);
                contents_.addChild(phoneImg_);
                contents_.addChild(emailImg_);
                contents_.addChild(addressImg_);
                contents_.addChild(noteImg_);
                contents_.addChild(birthday_);
                contents_.addChild(bdayExtras_);
                contents_.addChild(phone_);
                contents_.addChild(email_);
                contents_.addChild(address_);
                contents_.addChild(note_);
                image_.setTransparentColor(ColorRGB::Black());
                name_.setFont(Font::fromROM<assets::OpenDyslexic64>());
                bdayExtras_.setColor(ui::Style::accentFg());
                contextMenu_.add(ui::ActionMenu::Item("Edit name", [this]() {
                    auto n = App::run<TextDialog>(c_.name);
                    if (n.has_value()) {
                        c_.name = n.value();
                        name_.setText(c_.name);
                    }
                }));
                contextMenu_.add(ui::ActionMenu::Item("Edit birthday", [this]() {
                    auto d = App::run<DateDialog>(c_.birthday);
                    if (d.has_value()) {
                        c_.birthday = d.value();
                        birthday_.setText(STR(c_.birthday.day() << "/" << c_.birthday.month() << "/" << c_.birthday.year()));
                        bdayExtras_.setText(STR("(" << c_.daysTillBirthday() << " days)"));
                    }
                }));
                contextMenu_.add(ui::ActionMenu::Item("Edit phone", [this]() {
                    auto n = App::run<TextDialog>(c_.phone);
                    if (n.has_value()) {
                        c_.phone = n.value();
                        phone_.setText(c_.phone);
                    }
                }));
                contextMenu_.add(ui::ActionMenu::Item("Edit email", [this]() {
                    auto n = App::run<TextDialog>(c_.email);
                    if (n.has_value()) {
                        c_.email = n.value();
                        email_.setText(c_.email);
                    }
                }));
                contextMenu_.add(ui::ActionMenu::Item("Edit address", [this]() {
                    auto n = App::run<TextDialog>(c_.address);
                    if (n.has_value()) {
                        c_.address = n.value();
                        address_.setText(c_.address);
                    }
                }));
                contextMenu_.add(ui::ActionMenu::Item("Edit note", [this]() {
                    auto n = App::run<TextDialog>(c_.note);
                    if (n.has_value()) {
                        c_.note = n.value();
                        note_.setText(c_.note);
                    }
                }));
            }

            void setAnimation(Point iconStart, Point textStart, uint32_t durationMs = 500) {
                t_.setDuration(durationMs);
                image_.setPos(iconStart);
                name_.setPos(textStart);
                aImage_ = Animation2D{iconStart, Point{8, 18}, interpolation::cosine};
                aName_ = Animation2D{textStart, Point{80, 20}, interpolation::cosine};
                t_.start();
                contents_.setVisible(false);
            }

            void update() override {
                if (t_.running())
                    return;
                if (btnPressed(Btn::B)) {
                    btnClear(Btn::B);
                    if (t_.duration() != 0) {
                        exitAtEnd_ = true;
                        aImage_.reverse();
                        aName_.reverse();
                        t_.start();
                        contents_.setVisible(false);
                    } else {
                        exit();
                    }
                }
                if (btnPressed(Btn::Select)) {
                    auto action = App::run<PopupMenu<ui::Action>>(&contextMenu_);
                    if (action.has_value())
                        action.value()();
                }
                ui::Form<void>::update();
            }

            void draw() override {
                if (t_.running()) {
                    t_.update();
                    image_.setPos(aImage_.update(t_));
                    name_.setPos(aName_.update(t_));
                    if (! t_.running()) {
                        if (exitAtEnd_)
                            exit();
                        else
                            contents_.setVisible(true);
                    }
                }
                ui::Form<void>::draw();
            }

        protected:

        private:
            Contact & c_;
            ui::Image image_;
            ui::Label name_;
            ui::ScrollView contents_{Rect::XYWH(0, 90, 320, 150)};
            ui::Image bdayImg_{50, 0, Icon{assets::icons_24::birthday_cake}};
            ui::Label birthday_;
            ui::Label bdayExtras_;
            ui::Image phoneImg_{50, 30, Icon{assets::icons_24::poo}};
            ui::Label phone_;
            ui::Image emailImg_{50, 60, Icon{assets::icons_24::poo}};
            ui::Label email_;
            ui::Image addressImg_{50, 90, Icon{assets::icons_24::poo}};
            ui::Label address_;
            ui::Image noteImg_{50, 120, Icon{assets::icons_24::poo}};
            ui::Label note_;
            Timer t_{0};
            bool exitAtEnd_ = false;
            Animation2D aImage_;
            Animation2D aName_;

            ui::ActionMenu contextMenu_;
        }; // ContactViewer

        Friends() : App{} {}

    protected:

        friend class App;

        void draw() override {}

        void loop() override {
            //throw "foobar";
            ContactDialog cd{};
            // run the contact dialog in launcher mode with contact viewer as callback app
            cd.run([& cd](Contact c) {
                // show the contact viewer for the appropriate contact
                ContactViewer cv{c};
                cv.setAnimation(cd.iconPosition(), cd.textPosition());
                cv.loop();
                //App::run<ContactViewer>(c);
            });
        }
    }; // rckid::Friends
} // namespace rckid