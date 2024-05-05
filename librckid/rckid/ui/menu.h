#pragma once

#include "../rckid.h"

namespace rckid {

    /** Menu item, designed for minimal overhead. 
     
        Note that the menu item does *not* own its data (i.e. the icon or the text). This is fine for static purposes where the label and the icon are part of code, but will break when using dynamic data

        On carousel have events when showing/removing the items, etc. 
     */
    class MenuItem {
    public:
        char const * text; 
        uint8_t const * icon;
        size_t iconBytes;

        MenuItem(char const * text, uint8_t const * icon, size_t iconBytes):
            text{text}, icon{icon}, iconBytes{iconBytes} {}

        template<int SIZE>
        static MenuItem create(char const * text, uint8_t const (& icon)[SIZE]) {
            return MenuItem{text, icon, SIZE};
        }
    }; 

    /** */
    class Menu {
    public:

        Menu() = default;
        Menu(std::initializer_list<MenuItem> items): items_{std::move(items)} {}

        virtual ~Menu() = default;

        size_t size() const { return items_.size(); }

        MenuItem const & operator [] (size_t index) const { return items_[index]; }
        MenuItem & operator [] (size_t index) { return items_[index]; }

    private:
        std::vector<MenuItem> items_;
    }; 

}