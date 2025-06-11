#pragma once

#include <platform/tinydate.h>

#include "rckid.h"
#include "utils/json.h"

namespace rckid {

    /** Contact. 
     
        Basic contact information. As a child oriented device, we do not bother with names & surnames, everything is under single name. Contacts can be birthday and associated image, etc.
     */
    class Contact {
    public:
        /** Name of the contact. */
        String name;
        /** Birthday (time is ignored). */
        TinyDate birthday;
        /** Image used with the contact. This must be image from the device's SD card. */
        String image;
        /** Telegram ID for the contact, which is also used as a walkie-talkie identifier. */
        String id;

        String email;
        String phone;
        String address;
        String note;

        /** Creates the contact from given JSON object.
         */
        Contact(json::Object const & from) {
            if (from.has("name"))
                name = from["name"].asStringOrDefault("???");
            if (from.has("image"))
                image = from["image"].asStringOrDefault("icons/star.png");
            if (from.has("id"))
                id = from["id"].asStringOrDefault("");
            if (from.has("birthday"))
                birthday = TinyDate::fromRaw(static_cast<uint32_t>(from["birthday"].asIntegerOrDefault(0)));
            if (from.has("email"))
                email = from["email"].asStringOrDefault("");
            if (from.has("phone"))
                phone = from["phone"].asStringOrDefault("");
            if (from.has("address"))
                address = from["address"].asStringOrDefault("");
            if (from.has("note"))
                note = from["note"].asStringOrDefault("");
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
