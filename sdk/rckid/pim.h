#pragma once

#include <algorithm>
#include <vector>
#include <functional>

#include <platform/tinydate.h>

#include "rckid.h"
#include "utils/ini.h"
#include "ui/style.h"

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
        Icon image{assets::icons_64::happy_face};

        String email;
        String phone;
        String address;
        String note;
        /** Color identifying the contact. 
         */
        ColorRGB color = ui::Style::fg();
        ColorRGB bgColor = ui::Style::bg();

        int64_t telegramId = 0;

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
                } else if (kv->first == "color") {
                    color = ColorRGB::fromString(kv->second);
                } else if (kv->first == "bgColor") {
                    bgColor = ColorRGB::fromString(kv->second);
                } else if (kv->first == "telegramId") {
                    telegramId = std::atoll(kv->second.c_str());
                } else if (kv->first == "birthday") {
                    if (!birthday.setFromString(kv->second.c_str()))
                        LOG(LL_ERROR, "Invalid birthday format for contact " << name << ": " << kv->second);
                }
            }
        }

        Contact(String name):
            name{name},
            image{Icon{assets::icons_64::happy_face}} {
        }

        uint32_t daysTillBirthday() const {
            TinyDate today = timeNow().date;
            return today.daysTillNextAnnual(birthday);  
        }

        void save(ini::Writer & writer) const {
            writer.writeSection("contact");
            writer.writeValue("name", name);
            if (image.isFile())
                writer.writeValue("image", image.filename());
            writer.writeValue("birthday", STR(birthday.day() << "/" << birthday.month() << "/" << birthday.year()));
            writer.writeValue("email", email);
            writer.writeValue("phone", phone);
            writer.writeValue("address", address);
            writer.writeValue("note", note);
            if (color != ui::Style::fg())
                writer.writeValue("color", color.toString());
            if (bgColor != ui::Style::bg())
                writer.writeValue("bgColor", bgColor.toString());
            if (telegramId != 0)
                writer.writeValue("telegramId", STR(telegramId));
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

        static void saveAll(std::vector<Contact *> const & contacts) {
            ini::Writer writer{fs::fileWrite(CONTACTS_PATH)};
            for (Contact * c : contacts)
                c->save(writer);
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
    class Myself {
    public:

        static constexpr char const * MYSELF_PATH = "/myself.ini";

        static Contact & contact() {
            if (contact_ == nullptr)
                loadContact();
            return *contact_;
        }

        static String const & parentPassword() {
            return parentPassword_;
        }

        static void setParentPassword(String const & password) {
            parentPassword_ = password;
            save();
        }

        static void initialize() {
            if (fs::exists(MYSELF_PATH)) {
                ini::Reader reader{fs::fileRead(MYSELF_PATH)};
                while (auto section = reader.nextSection()) {
                    if (section.value() == "contact") {
                        if (contact_ == nullptr)
                            contact_ = new Contact{reader};
                    } else if (section.value() == "settings") {
                        while (std::optional<std::pair<String, String>> kv = reader.nextValue()) {
                            if (kv->first == "parentPassword") {
                                parentPassword_ = kv->second;
                            } else if (kv->first == "dailyBudget") {
                                uint32_t b = static_cast<uint32_t>(std::atoi(kv->second.c_str()));
                                budgetDailySet(b);
                            } else {
                                LOG(LL_ERROR, "Unknown myself setting: " << kv->first);
                            }
                        }
                    } else {
                        LOG(LL_ERROR, "Invalid myself section: " << section.value());
                    }
                }
            }
            if (contact_ == nullptr)
                contact_ = new Contact{"Myself"};
        }

        static void save() {
            ini::Writer writer{fs::fileWrite(MYSELF_PATH)};
            contact_->save(writer);
            writer.writeSection("settings");
            if (!parentPassword_.empty())
                writer.writeValue("parentPassword", parentPassword_);
            writer.writeValue("dailyBudget", STR(budgetDaily()));
        }

    protected:

        static void loadContact() {
            if (fs::exists(MYSELF_PATH)) {
                ini::Reader reader{fs::fileRead(MYSELF_PATH)};
                while (auto section = reader.nextSection()) {
                    if (section.value() == "myself") {
                        if (contact_ == nullptr)
                            contact_ = new Contact{reader};
                    }
                }
            }
            if (contact_ == nullptr)
                contact_ = new Contact{"Myself"};
        }

    protected:
        static inline String parentPassword_; 
        static inline Contact * contact_ = nullptr;

    }; // rckid::User


} // namespace rckid
