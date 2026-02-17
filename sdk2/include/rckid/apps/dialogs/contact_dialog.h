#pragma once

#include <rckid/contacts.h>
#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>

#include <assets/OpenDyslexic64.h>
#include <assets/Iosevka24.h>
#include <assets/icons_24.h>

namespace rckid {

    class ContactDialog : public ui::App<bool> {
    public:

        String name() const override { return "Contact"; }

        ContactDialog(Contact * c) : 
            ui::App<bool>{Rect::XYWH(0, 0, 320, 240)},
            c_{c}
        {
            using namespace ui;
            image_ = addChild(new ui::Image{})
                << SetBitmap(c->image)
                << SetRect(Rect::XYWH(0, 20, 96, 64));
            name_ = addChild(new ui::Label{})
                << SetText(c->name)
                << SetRect(Rect::XYWH(95, 20, 220, 64))
                << SetFont(assets::OpenDyslexic64);

            bdayImg_ = addChild(new ui::Image{})
                << SetBitmap(assets::icons_24::birthday_cake)
                << SetRect(Rect::XYWH(0, 90, 80, 24))
                << SetHAlign(HAlign::Right);
            bday_ = addChild(new ui::Label{})
                << SetText(STR(c->birthday.day() << "/" << c->birthday.month() << "/" << c->birthday.year()))
                << SetFont(assets::Iosevka24)
                << SetRect(Rect::XYWH(95, 90, 220, 24));

            phoneImg_ = addChild(new ui::Image{})
                << SetBitmap(assets::icons_24::phone)
                << SetRect(Rect::XYWH(0, 120, 80, 24))
                << SetHAlign(HAlign::Right);
            phone_ = addChild(new ui::Label{})
                << SetText(c->phone)
                << SetFont(assets::Iosevka24)
                << SetRect(Rect::XYWH(95, 120, 220, 24));

            emailImg_ = addChild(new ui::Image{})
                << SetBitmap(assets::icons_24::email)
                << SetRect(Rect::XYWH(0, 150, 80, 24))
                << SetHAlign(HAlign::Right);
            email_ = addChild(new ui::Label{})
                << SetText(c->email)
                << SetFont(assets::Iosevka24)
                << SetRect(Rect::XYWH(95, 150, 220, 24));    

            addressImg_ = addChild(new ui::Image{})
                << SetBitmap(assets::icons_24::house)
                << SetRect(Rect::XYWH(0, 180, 80, 24))
                << SetHAlign(HAlign::Right);
            address_ = addChild(new ui::Label{})
                << SetText(c->address)
                << SetFont(assets::Iosevka24)
                << SetRect(Rect::XYWH(95, 180, 220, 24));

            noteImg_ = addChild(new ui::Image{})
                << SetBitmap(assets::icons_24::bookmark)
                << SetRect(Rect::XYWH(0, 210, 80, 24))
                << SetHAlign(HAlign::Right);
            note_ = addChild(new ui::Label{})
                << SetText(c->note)
                << SetFont(assets::Iosevka24)
                << SetRect(Rect::XYWH(95, 210, 220, 24));
        }

    protected:

        void onLoopStart() override {
            ui::App<bool>::onLoopStart();
            animate()
                << ui::FlyIn(note_, 300)
                << ui::FlyIn(noteImg_, 300)
                << ui::FlyIn(address_, 300)->setDelayMs(50)
                << ui::FlyIn(addressImg_, 300)->setDelayMs(50)
                << ui::FlyIn(email_, 300)->setDelayMs(100)
                << ui::FlyIn(emailImg_, 300)->setDelayMs(100)
                << ui::FlyIn(phone_, 300)->setDelayMs(50)
                << ui::FlyIn(phoneImg_, 300)->setDelayMs(150)
                << ui::FlyIn(bday_, 300)->setDelayMs(200)
                << ui::FlyIn(bdayImg_, 300)->setDelayMs(200)
                << ui::FlyIn(name_, 300)->setDelayMs(250)
                << ui::FlyIn(image_, 300)->setDelayMs(250);
        }

        void loop() {
            ui::App<bool>::loop();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                exit(std::move(dirty_));
                waitUntilIdle();
                animate()
                    << ui::FlyOut(note_, 300)->setDelayMs(250)
                    << ui::FlyOut(noteImg_, 300)->setDelayMs(250)
                    << ui::FlyOut(address_, 300)->setDelayMs(200)
                    << ui::FlyOut(addressImg_, 300)->setDelayMs(200)
                    << ui::FlyOut(email_, 300)->setDelayMs(150)
                    << ui::FlyOut(emailImg_, 300)->setDelayMs(150)
                    << ui::FlyOut(phone_, 300)->setDelayMs(100)
                    << ui::FlyOut(phoneImg_, 300)->setDelayMs(100)
                    << ui::FlyOut(bday_, 300)->setDelayMs(50)
                    << ui::FlyOut(bdayImg_, 300)->setDelayMs(50)
                    << ui::FlyOut(name_, 300)->setDelayMs(0)
                    << ui::FlyOut(image_, 300)->setDelayMs(0);

                waitUntilIdle();
            }
        }

    private:

        Contact * c_;
        bool dirty_ = false;

        ui::Image * image_;
        ui::Label * name_;
        ui::Image * bdayImg_;
        ui::Label * bday_;
        ui::Label * bdayExtras_;
        ui::Image * phoneImg_;
        ui::Label * phone_;
        ui::Image * emailImg_;
        ui::Label * email_;
        ui::Image * addressImg_;
        ui::Label * address_;
        ui::Image * noteImg_;
        ui::Label * note_;

    };

} // namespace rckid