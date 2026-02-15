#include <rckid/pim.h>

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
    
} // namespace rckid