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

        static uint32_t readAll(std::function<void(unique_ptr<Contact>)> callback);


        template<typename It>
        static void saveAll(It begin, It end) {
            auto f = fs::writeFile(CONTACTS_PATH);
            if (f == nullptr) {
                LOG(LL_ERROR, "Failed to open contacts file for writing");
                return;
            }
            ini::Writer writer{*f};
            for (auto it = begin; it != end; ++it) {
                Contact * c = it->get();
                c->save(writer);
            }
        }

        static constexpr char const * CONTACTS_PATH = "/contacts.ini";
    }; // rckid::Contact

} // namespace rckid