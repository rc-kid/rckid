#pragma once

#include <vector>

#include "../rckid.h"
#include "../graphics/bitmap.h"

namespace rckid::ui {

    /** Menu representation.

        The UI decouples the menu itself, represented by this class from the actual rendering of the menu items, which is done by widgets such as the Carousel. 

        Item = just fillText and fillIcon 
        Action = void * payload
        Submenu = generator payload
     */
    class Menu {
    public:
        /** Basic item of a menu.
         
            Each menu items has a text and an optional icon. The menu item can also have a payload, which is a 32-bit value that can be used to store additional information about the item.
         */
        class Item {
        public:

            virtual ~Item() = default;

            uint32_t payload() const { return payload_; }
            void setPayload(uint32_t value) { payload_ = value; }

            virtual void fillText(std::string & text) const = 0;

            virtual bool fillIcon([[maybe_unused]] Bitmap<16> & bmp) const { return false; }

        private:

            uint32_t payload_ = 0;
        }; // rckid::ui::Menu::Item

        /** Static menu item with no memory allocation for its contents. This is done by using string literals as text and pointers to images stored in ROM as icons. Note that while the item itself thus does not need any allocations, displaying the item might require allocations for updating the text or setting/decoding the icon. 
         */
        class StaticItem : public Item {
        public:

            void fillText(std::string & text) const override { text = text_; }

            bool fillIcon(Bitmap<16> &bmp) const override {
                if (iconData_ == nullptr)
                    return false;
                bmp.loadImage(PNG::fromBuffer(iconData_, iconSize_));
                return true;
            }

        private:
            char const * text_ = nullptr;
            uint8_t const * iconData_ = nullptr;
            uint32_t iconSize_ = 0;
        }; // rckid::ui::Menu::StaticItem


        Menu() = default;

        Menu(std::initializer_list<Item *> items):
            items_{items} {
        }

        Menu(Menu && other): items_{std::move(other.items_)} {
        }

        ~Menu() {
            for (auto & mi : items_)
                delete mi;
        }

        uint32_t size() const { return items_.size(); }

        Item & operator [] (uint32_t index) {
            ASSERT(index < items_.size());
            return *(items_[index]);
        }

        void add(Item * item) {
            items_.push_back(item);
        }

        void clear() {
            for (auto mi : items_)
                delete mi;
            items_.clear();
        }

    private:

        std::vector<Item *> items_;
    }; // rckid::ui::Menu

} // namespace rckid::ui