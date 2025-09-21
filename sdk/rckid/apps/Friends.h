#pragma once

#include "dialogs/ContactDialog.h"
#include "../ui/scrollview.h"

namespace rckid {

    /** The friends app is a simple ContactDialog in a launcher mode where the launched app is contact viewer & editor. 
     */
    class Friends : public App {
    public:

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
                birthday_{80, 0, STR(c.birthday.day() << " / " << c.birthday.month() << " / " << c.birthday.year())} {
                g_.addChild(image_);
                g_.addChild(name_);
                g_.addChild(contents_);
                contents_.addChild(bdayImg_);
                contents_.addChild(phoneImg_);
                contents_.addChild(emailImg_);
                contents_.addChild(addressImg_);
                contents_.addChild(noteImg_);
                contents_.addChild(birthday_);
                image_.setTransparentColor(ColorRGB::Black());
                name_.setFont(Font::fromROM<assets::OpenDyslexic64>());
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