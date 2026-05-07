#pragma once

#include <rckid/contacts.h>
#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>
#include <rckid/apps/dialogs/popup_menu.h>
#include <rckid/apps/dialogs/text_dialog.h>
#include <rckid/apps/dialogs/color_dialog.h>
#include <rckid/apps/dialogs/file_dialog.h>

#include <assets/OpenDyslexic64.h>
#include <assets/Iosevka24.h>
#include <assets/icons_24.h>
#include <assets/icons_16.h>

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
                << SetFg(c->color)
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

            contextMenu_
                << MenuItem("Edit name", assets::icons_16::letter_a, [this](){
                        auto name = App::run<TextDialog>(c_->name);
                        if (name) {
                            c_->name = name.value();
                            name_->setText(c_->name);
                            dirty_ = true;
                        }
                    })
                << MenuItem("Select image", assets::icons_16::picture, [this](){
                        auto img = App::run<FileDialog>("files/icons");
                        if (img) {
                            c_->image = ImageSource{img.value()};
                            ui::with(image_)
                                << SetBitmap(c_->image);
                            dirty_ = true;
                        }
                    })
                << MenuItem("Set birthday", assets::icons_16::birthday_cake, [this](){
                        // TODO
                    })
                << MenuItem("Set phone", assets::icons_16::phone, [this](){
                        auto x = App::run<TextDialog>(c_->phone);
                        if (x) {
                            c_->phone = x.value();
                            phone_->setText(c_->phone);
                            dirty_ = true;
                        }
                    })
                << MenuItem("Set email", assets::icons_16::email, [this](){
                        auto x = App::run<TextDialog>(c_->email);
                        if (x) {
                            c_->email = x.value();
                            email_->setText(c_->email);
                            dirty_ = true;
                        }
                    })
                << MenuItem("Set address", assets::icons_16::house, [this](){
                        auto x = App::run<TextDialog>(c_->address);
                        if (x) {
                            c_->address = x.value();
                            address_->setText(c_->address);                            
                            dirty_ = true;
                        }
                    })
                << MenuItem("Set note", assets::icons_16::bookmark, [this](){
                        auto x = App::run<TextDialog>(c_->note);
                        if (x) {
                            c_->note = x.value();
                            note_->setText(c_->note);
                            dirty_ = true;
                        }
                    })
                << MenuItem("Set color", assets::icons_16::light, [this](){
                        auto x = App::run<ColorDialog>(c_->color);
                        if (x) {
                            c_->color = x.value();
                            name_->setFg(c_->color);
                            dirty_ = true;
                        }
                    });
        }

    protected:

        void onLoopStart() override {
            ui::App<bool>::onLoopStart();
            root_.flyIn();
        }

        void loop() {
            ui::App<bool>::loop();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                exit(std::move(dirty_));
                // wait for idle to make sure we are exiting from known state
                waitUntilIdle();
                root_.flyOut();
                waitUntilIdle();
            }
            if (btnPressed(Btn::Select)) {
                auto action = App::run<PopupMenu>(contextMenu_);
                if (action)
                    action.value()->action()();
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

        ui::Menu contextMenu_;

    };

} // namespace rckid