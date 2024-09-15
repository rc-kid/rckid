#pragma once

#include <optional>

#include "../rckid.h"
#include "../graphics/bitmap.h"
#include "../graphics/png.h"

namespace rckid {

    /** Menu item

        The class is most an API specification for a menu item with interface for displaying      
        - enabled
        - text 
        - drawIcon
        - action (payload)
        - release to release any unnecessary resources (when no longer displayed, etc)

     */
    class MenuItem {
    public:
        /** Returns true if the menu item is enabled. 
         
            Default implementation always returns true. 
         */
        virtual bool enabled() const { return true; }

        /** Returns the menu item text as a C-string. 
         
            C strings are used so that static menu items that read values directly do not have to create copies into std::string. 
         */
        virtual char const * text() const = 0;

        /** Returns an icon to be used with the menu */
        virtual std::optional<Surface<ColorRGB>> icon() const { return std::nullopt; }



        /** Returns the payload associated with the menu item. 
         
            DO WE NEED THIS? 
         */
        void * payload() const { return payload_; }

        virtual ~MenuItem() noexcept = default;

    protected:

        MenuItem(void * payload): payload_{payload} {}

        void * payload_ = nullptr;

    }; // rckid::MenuItem

    /** Menu container API 
     
    */
    class Menu {
    public:

        Menu(std::initializer_list<MenuItem *> items):
            items_{items} {
        }

        ~Menu() {
            for (auto & mi : items_)
                delete mi;
        }

        uint32_t size() const { return items_.size(); }

        MenuItem & operator [] (uint32_t index) {
            ASSERT(index < items_.size());
            return *items_[index];
        }

        void add(MenuItem * item) {
            items_.push_back(item);
        }

    private:

        std::vector<MenuItem *> items_;
    }; // rckid::Menu







    class StaticMenuItem : public MenuItem {
    public:

        StaticMenuItem(char const * text, void * payload = nullptr):
            MenuItem{payload},
            text_{text} {
        }

        StaticMenuItem(char const * text, uint8_t const * iconData, uint32_t iconSize, void * payload = nullptr):
            MenuItem{payload},
            text_{text}, 
            iconData_{iconData},
            iconSize_{iconSize} {
        }

        template<uint32_t SIZE>
        StaticMenuItem(char const * text, uint8_t const (&buffer)[SIZE], void * payload = nullptr):
            MenuItem{payload_},
            text_{text}, 
            iconData_{buffer},
            iconSize_{SIZE} {
        }

        bool enabled() const override { return enabled_; }

        char const * text() const override { return text_; }

        std::optional<Surface<ColorRGB>> icon() const override {
            if (iconData_ == nullptr)
                return std::nullopt;
            return Surface<ColorRGB>::fromImage(PNG::fromBuffer(iconData_, iconSize_));
        }

        void enable(bool value) { enabled_ = value; }

    private:
        bool enabled_ = true;
        char const * text_ = nullptr;
        uint8_t const * iconData_ = nullptr;
        uint32_t iconSize_ = 0;

    }; // rckid::StaticMenuItem


} // namespace rckid