#include <rckid/contacts.h>

#include <assets/icons_64.h>

namespace rckid {

    Contact::Contact(String name):
        name{name},
        image{ImageSource{assets::icons_64::happy_face}} {
    }

    Contact::Contact(ini::Reader & reader):
        image{ImageSource{assets::icons_64::happy_face}}
     {
        reader 
            >> ini::Field("name", name)
            >> ini::Field("birthday", birthday)
            >> ini::Field("email", email)
            >> ini::Field("phone", phone)
            >> ini::Field("address", address)
            >> ini::Field("note", note)
            >> ini::Field("color", color)
            >> ini::Field("bgColor", bgColor)
            >> ini::Field("telegramId", telegramId);
    }

    void Contact::save(ini::Writer & writer) const {
        writer 
            << ini::Section("contact")
                << ini::Field("name", name)
                //<< ini::Field("image", image)
                << ini::Field("birthday", birthday)
                << ini::Field("email", email)
                << ini::Field("phone", phone)
                << ini::Field("address", address)
                << ini::Field("note", note)
                << ini::Field("color", color)
                << ini::Field("bgColor", bgColor)
                << ini::Field("telegramId", telegramId);
    }

    uint32_t Contact::readAll(std::function<void(unique_ptr<Contact>)> callback) {
        if (!fs::exists(CONTACTS_PATH))
            return 0;
        auto f = fs::readFile(CONTACTS_PATH);
        if (f == nullptr)
            return 0;
        ini::Reader reader{*f};
        uint32_t count = 0;
        reader 
            >> ini::SectionArray("contact", [&](ini::Reader & r) {
                callback(std::make_unique<Contact>(r));
                ++count;
            });
        return count;
    }

    
} // namespace rckid