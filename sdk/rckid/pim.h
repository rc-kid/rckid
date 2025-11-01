#pragma once

#include <algorithm>
#include <vector>
#include <functional>

#include <platform/tinydate.h>

#include "rckid.h"
#include "utils/ini.h"

namespace rckid {

    /** Contact. 
     
        Basic contact information. As a child oriented device, we do not bother with names & surnames, everything is under single name. Contacts can be birthday and associated image, etc.
     */
    class Contact{
    public:

        static constexpr char const * CONTACTS_PATH = "/contacts.ini";

        /** Name of the contact. */
        String name;
        /** Birthday (time is ignored). */
        TinyDate birthday;
        /** Image used with the contact. This must be image from the device's SD card. */
        String image;

        String email;
        String phone;
        String address;
        String note;

        Contact(ini::Reader & reader) {
            while (std::optional<std::pair<String, String>> kv = reader.nextValue()) {
                if (kv->first == "name") {
                    name = kv->second;
                } else if (kv->first == "image") {
                    image = kv->second;
                } else if (kv->first == "email") {
                    email = kv->second;
                } else if (kv->first == "phone") {
                    phone = kv->second;
                } else if (kv->first == "address") {
                    address = kv->second;
                } else if (kv->first == "note") {
                    note = kv->second;
                } else if (kv->first == "birthday") {
                    if (!birthday.setFromString(kv->second.c_str()))
                        LOG(LL_ERROR, "Invalid birthday format for contact " << name << ": " << kv->second);
                }
            }
        }

        uint32_t daysTillBirthday() const {
            TinyDate today = timeNow().date;
            return today.daysTillNextAnnual(birthday);  
        }

        void save(ini::Writer & writer) const {
            writer.writeSection("contact");
            writer.writeValue("name", name);
            writer.writeValue("image", image);
            writer.writeValue("birthday", STR(birthday.day() << "/" << birthday.month() << "/" << birthday.year()));
            writer.writeValue("email", email);
            writer.writeValue("phone", phone);
            writer.writeValue("address", address);
            writer.writeValue("note", note);
        }

        static void forEach(std::function<void(Contact)> f) {
            if (fs::exists(CONTACTS_PATH)) {
                ini::Reader reader{fs::fileRead(CONTACTS_PATH)};
                while (auto section = reader.nextSection()) {
                    if (section.value() != "contact") {
                        LOG(LL_ERROR, "Invalid contact section: " << section.value());
                        continue;
                    }
                    f(Contact{reader});
                }
            }
        }

        static std::optional<Contact> getNearestBirthday() {
            std::optional<Contact> nearest;
            uint32_t minDays = 366;
            forEach([&](Contact c) {
                uint32_t days = c.daysTillBirthday();
                if (days < minDays) {
                    minDays = days;
                    nearest = c;
                }
            });
            return nearest;
        }

        static void saveAll(std::vector<Contact> const & contacts) {
            ini::Writer writer{fs::fileWrite(CONTACTS_PATH)};
            for (auto const & c : contacts)
                c.save(writer);
        }

    }; // rckid::Contact

    class Holiday {
    public:
        static constexpr char const * HOLIDAY_PATH = "/holidays.ini";

        String name;
        TinyDate date;
        String image;

        Holiday(ini::Reader & reader) {
            while (std::optional<std::pair<String, String>> kv = reader.nextValue()) {
                if (kv->first == "name") {
                    name = kv->second;
                } else if (kv->first == "image") {
                    image = kv->second;
                } else if (kv->first == "date") {
                    if (!date.setFromString(kv->second.c_str()))
                        LOG(LL_ERROR, "Invalid date format for holiday " << name << ": " << kv->second);
                } else if (kv->first == "algorithm") {
                    // TODO not to do with this
                } else {
                    LOG(LL_ERROR, "Unknown holiday property " << kv->first);
                }       
            }
        }

        uint32_t daysTillHoliday() const {
            TinyDate today = timeNow().date;
            return today.daysTillNextAnnual(date);  
        }

        static void forEach(std::function<void(Holiday)> f) {
            if (fs::exists(HOLIDAY_PATH)) {
                ini::Reader reader{fs::fileRead(HOLIDAY_PATH)};
                while (auto section = reader.nextSection()) {
                    if (section.value() != "date") {
                        LOG(LL_ERROR, "Invalid holiday section: " << section.value());
                        continue;
                    }
                    f(Holiday{reader});
                }
            }
        }

    }; // rckid::Holiday

    /** User of the device. 
     
        The PIM information for the current user is always stored in the AVR's EEPROM and RAM so that even power cycles / SD card changes keep the data available. 

        Aside from other contacts, the user also has the game-specific information, such as global coins and remaining screen time. 

     */
    class User : public Contact {
    public:




    protected:
        uint32_t remainingScreenTime_ = 0; 
        uint32_t coins_ = 0;

    }; // rckid::User


} // namespace rckid
