#pragma once

#include <platform/tinydate.h>

#include "rckid.h"
#include "utils/ini.h"

namespace rckid {

    /** Contact. 
     
        Basic contact information. As a child oriented device, we do not bother with names & surnames, everything is under single name. Contacts can be birthday and associated image, etc.
     */
    class Contact {
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

    }; // rckid::Contact

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
