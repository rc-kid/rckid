#pragma once

#include "dialogs/ContactDialog.h"
#include "dialogs/TextDialog.h"
#include "dialogs/DateDialog.h"
#include "../ui/scrollview.h"
#include "../ui/carousel.h"
#include "../assets/fonts/Iosevka24.h"


namespace rckid {

    /** The friends app is a simple ContactDialog in a launcher mode where the launched app is contact viewer & editor. 
     */
    class Friends : public ui::Form<void> {
    public:

        /** Signle Contact Viewer and editor. 
         
            Displays the contact information and allows its eedits. 
         */
        class ContactViewer : public ui::Form<bool> {
        public:

            /** Use umbrella name for all friends apps.
             */
            String name() const override { return "Friends"; }

            ContactViewer(Contact & c) : 
                ui::Form<bool>{Rect::XYWH(0, 0, 320, 240)},
                c_{c},
                image_{8, 18, c.image},
                name_{80, 20, c.name},
                birthday_{80, 0, STR(c.birthday.day() << "/" << c.birthday.month() << "/" << c.birthday.year())},
                bdayExtras_{200, 0, STR("(" << c.daysTillBirthday() << " days)")},
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
                birthday_.setFont(Font::fromROM<assets::Iosevka24>());
                phone_.setFont(Font::fromROM<assets::Iosevka24>());
                email_.setFont(Font::fromROM<assets::Iosevka24>());
                address_.setFont(Font::fromROM<assets::Iosevka24>());
                note_.setFont(Font::fromROM<assets::Iosevka24>());
                bdayExtras_.setFont(Font::fromROM<assets::Iosevka24>());
                image_.setTransparentColor(ColorRGB::Black());
                name_.setFont(Font::fromROM<assets::OpenDyslexic64>());
                bdayExtras_.setColor(ui::Style::accentFg());
                contextMenu_.add(ui::ActionMenu::Item("Edit name", [this]() {
                    auto n = App::run<TextDialog>(c_.name);
                    if (n.has_value()) {
                        c_.name = n.value();
                        name_.setText(c_.name);
                        setResult(true); // mark dirty
                    }
                }));
                contextMenu_.add(ui::ActionMenu::Item("Edit image", [this]() {
                    auto icon = App::run<FileDialog>("/files/icons", "Select friend icon");
                    if (icon.has_value()) {
                        c_.image = Icon{icon.value().c_str()};
                        image_ = c_.image;
                        setResult(true); // mark dirty
                    }
                }));
                contextMenu_.add(ui::ActionMenu::Item("Edit birthday", [this]() {
                    auto d = App::run<DateDialog>(c_.birthday);
                    if (d.has_value()) {
                        c_.birthday = d.value();
                        birthday_.setText(STR(c_.birthday.day() << "/" << c_.birthday.month() << "/" << c_.birthday.year()));
                        bdayExtras_.setText(STR("(" << c_.daysTillBirthday() << " days)"));
                        setResult(true); // mark dirty
                    }
                }));
                contextMenu_.add(ui::ActionMenu::Item("Edit phone", [this]() {
                    auto n = App::run<TextDialog>(c_.phone);
                    if (n.has_value()) {
                        c_.phone = n.value();
                        phone_.setText(c_.phone);
                        setResult(true); // mark dirty
                    }
                }));
                contextMenu_.add(ui::ActionMenu::Item("Edit email", [this]() {
                    auto n = App::run<TextDialog>(c_.email);
                    if (n.has_value()) {
                        c_.email = n.value();
                        email_.setText(c_.email);
                        setResult(true); // mark dirty
                    }
                }));
                contextMenu_.add(ui::ActionMenu::Item("Edit address", [this]() {
                    auto n = App::run<TextDialog>(c_.address);
                    if (n.has_value()) {
                        c_.address = n.value();
                        address_.setText(c_.address);
                        setResult(true); // mark dirty
                    }
                }));
                contextMenu_.add(ui::ActionMenu::Item("Edit note", [this]() {
                    auto n = App::run<TextDialog>(c_.note);
                    if (n.has_value()) {
                        c_.note = n.value();
                        note_.setText(c_.note);
                        setResult(true); // mark dirty
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
                ui::Form<bool>::update();
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
                ui::Form<bool>::draw();
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
            ui::Image phoneImg_{50, 30, Icon{assets::icons_24::phone}};
            ui::Label phone_;
            ui::Image emailImg_{50, 60, Icon{assets::icons_24::email}};
            ui::Label email_;
            ui::Image addressImg_{50, 90, Icon{assets::icons_24::house}};
            ui::Label address_;
            ui::Image noteImg_{50, 120, Icon{assets::icons_24::bookmark}};
            ui::Label note_;
            Timer t_{0};
            bool exitAtEnd_ = false;
            Animation2D aImage_;
            Animation2D aName_;
            ui::ActionMenu contextMenu_;
        }; // Friends::ContactViewer

        String name() const override { return "Friends"; }

        Friends():
            ui::Form<void>{},
            c_{
                [this](){ return contacts_.size(); },
                [this](uint32_t index, Direction direction) {
                    Contact const & contact = contacts_[index];
                    c_.set(contact.name, contact.image, direction);
                }
            }
        {
            g_.addChild(c_);
            c_.setRect(Rect::XYWH(0, 160, 320, 80));
            c_.setFont(Font::fromROM<assets::OpenDyslexic64>());
            // fill in the contacts list
            Contact::forEach([this](Contact c) {
                contacts_.push_back(c);
            });
            refreshNextBirthdays();
            sort();

            contextMenu_.add(ui::ActionMenu::Item("Add contact", [this]() {
                auto name = App::run<TextDialog>("");
                if (! name.has_value())
                    return;
                Contact c{name.value()};
                c.birthday = timeNow().date;
                contacts_.push_back(c);
                Contact::saveAll(contacts_);
                c_.setItem(contacts_.size() - 1, Direction::None);
                editCurrentContact();
                sort(contacts_.back().name.c_str());
            }));
            contextMenu_.add(ui::ActionMenu::Item("Delete contact", [this]() {
                if (contacts_.size() == 0)
                    return;
                contacts_.erase(contacts_.begin() + c_.currentIndex());
                Contact::saveAll(contacts_);
                c_.setItem(0, Direction::Up);
                refreshNextBirthdays();
                sort();

            }));
        }


    protected:

        friend class App;

        void refreshNextBirthdays() {
            for (NextBirthday & nb : nextBirthdays_) {
                nb.numDays_ = 366;
                nb.icon_.setVisible(true);
                nb.label_.setVisible(true);
            }
            for (uint32_t i = 0; i < NUM_NEXT_BIRTHDAYS; ++i) {
                g_.addChild(nextBirthdays_[i].icon_);
                g_.addChild(nextBirthdays_[i].label_);
            }
            // fill next birthdays list
            for (Contact & c : contacts_) {
                uint32_t days = c.daysTillBirthday();
                for (uint32_t i = 0; i < NUM_NEXT_BIRTHDAYS; ++i) {
                    if (days < nextBirthdays_[i].numDays_) {
                        // insert here
                        for (uint32_t j = NUM_NEXT_BIRTHDAYS - 1; j > i; --j) {
                            nextBirthdays_[j].label_.setText(nextBirthdays_[j - 1].label_.text());
                            nextBirthdays_[j].numDays_ = nextBirthdays_[j - 1].numDays_;
                        }
                        nextBirthdays_[i].numDays_ = days;
                        nextBirthdays_[i].label_.setText(c.name);
                        break;
                    } else if (days == nextBirthdays_[i].numDays_) {
                        nextBirthdays_[i].label_.setText(STR(nextBirthdays_[i].label_.text() << ", " << c.name));
                        break;
                    }
                }
            }
            for (NextBirthday & nb: nextBirthdays_) {
                if (nb.numDays_ >= 366) {
                    nb.icon_.setVisible(false);
                    nb.label_.setVisible(false);
                } else {
                    nb.label_.setText(STR(nb.label_.text() << " (" << nb.numDays_ << " days)"));
                }
            }
        }

        /** Sorts the contact, selecting the given name when done. This is done by comparing the c_str pointers, which will not affected by the sort.
         */
        void sort(char const * currentName = nullptr) {
            std::sort(contacts_.begin(), contacts_.end(), [](Contact const & a, Contact const & b) {
                return a.name < b.name;
            });
            if (currentName != nullptr) {
                for (uint32_t i = 0; i < contacts_.size(); ++i) {
                    if (contacts_[i].name.c_str() == currentName) {
                        c_.setItem(i, Direction::None);
                        break;
                    }
                }
            }
        }

        void update() override {
            ui::Form<void>::update();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                return exit();
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                btnClear(Btn::A);
                btnClear(Btn::Up);
                editCurrentContact();
            }
            if (btnPressed(Btn::Select)) {
                btnClear(Btn::Select);
                auto action = App::run<PopupMenu<ui::Action>>(&contextMenu_);
                if (action.has_value())
                    action.value()();
            }
            c_.processEvents();
        }

        void focus() override {
            ui::Form<void>::focus();
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

        void editCurrentContact() {
            if (contacts_.size() != 0) {
                Contact & c = contacts_[c_.currentIndex()];
                ContactViewer cv{const_cast<Contact&>(c)};
                cv.setAnimation(c_.iconPosition(), c_.textPosition());
                cv.loop();
                c_.setItem(c_.currentIndex(), Direction::None);
                if (cv.result().has_value() && cv.result().value()) {
                    // we have edited the contact, so we need to save the contacts we have now
                    Contact::saveAll(contacts_);
                    refreshNextBirthdays();
                    sort(contacts_[c_.currentIndex()].name.c_str());
                }
            }
        }

    private:

        static constexpr uint32_t NUM_NEXT_BIRTHDAYS = 3;

        class NextBirthday {
        public:
            ui::Image icon_;
            ui::Label label_;
            uint32_t numDays_ = 366;

            NextBirthday(Coord index):
                icon_{8, 18 + index * 32, Icon{assets::icons_24::birthday_cake}},
                label_{40, 18 + index * 32, ""}
            {
                label_.setFont(Font::fromROM<assets::OpenDyslexic24>());
            }
        }; 


        ui::EventBasedCarousel c_;

        ui::ActionMenu contextMenu_;


        NextBirthday nextBirthdays_[NUM_NEXT_BIRTHDAYS] = {
            NextBirthday(0),
            NextBirthday(1),
            NextBirthday(2),
        };
        std::vector<Contact> contacts_;




        bool firstRun_ = true;

    }; // rckid::Friends

} // namespace rckid