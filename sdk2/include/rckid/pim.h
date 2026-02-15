#pragma once

#include <rckid/rckid.h>
#include <rckid/ini.h>
#include <rckid/filesystem.h>
#include <rckid/graphics/image_source.h>

namespace rckid {

    class Contact {
    public:

        String name;
        TinyDate birthday;
        ImageSource image;

        String email;
        String phone;
        String address;
        String note;

        Color color;
        Color bgColor;

        int64_t telegramId;

        Contact(String name);

        Contact(ini::Reader & reader);

        uint32_t daysTillBirthday() const {
            TinyDate today = time::now().date;
            return today.daysTillNextAnnual(birthday);
        }

        void save(ini::Writer & writer) const;

    }; // rckid::Contact

} // namespace rckid