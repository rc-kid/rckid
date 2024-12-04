#pragma once

#include <optional>

#include "../rckid.h"
#include "../graphics/bitmap.h"
#include "../graphics/png.h"

namespace rckid {

    /** Menu item
        
        A very simple menu item interface. Each menu item can return its text, given as C string (managed by the menu item itself) and optionally an icon, which is returned as RGB surface (managed by the caller). 

        Based on how the text & icon are stored/generated different menu item subclasses, such as StaticMenuItem are used. 
     */
    class MenuItem {
    public:
        /** Loads the text associated with the menu item to the provides string.

            This is so that the menu items do not have to worry about allocation of the strings, which is provided by their users.  
         */
        virtual void text(std::string & text) const = 0;

        /** Loads menu item icon into the given bitmap. 
         
            Returns true on success, false otherwise. 

            TODO the bitmap has to be at least as large as the icon stored with the menu for now, but we should add a scaling variant. 
         */
        virtual bool icon([[maybe_unused]] Bitmap<ColorRGB> & bmp) const { return false; }

        virtual ~MenuItem() = default;

        void * payloadPtr() const { return payloadPtr_; }

        void setPayloadPtr(void * value) { payloadPtr_ = value; }

        uint32_t payload() const { return payload_; }
        void setPayload(uint32_t value) { payload_ = value; }

    protected:

        MenuItem(uint32_t payload = 0, void * payloadPtr = nullptr): payload_{payload}, payloadPtr_{payloadPtr} {}

    private:

        uint32_t payload_ = 0;
        void * payloadPtr_ = nullptr;

    }; // rckid::MenuItem

    /** Static menu item with no memory allocation for its contents. 
     
        The static menu item is useful for menu items where 
     */
    class StaticMenuItem : public MenuItem {
    public:

        StaticMenuItem(char const * text, uint32_t payload = 0, void * payloadPtr = nullptr):
            MenuItem{payload, payloadPtr},
            text_{text} {
        }

        StaticMenuItem(char const * text, uint8_t const * iconData, uint32_t iconSize, uint32_t payload = 0, void * payloadPtr = nullptr):
            MenuItem{payload, payloadPtr},
            text_{text}, 
            iconData_{iconData},
            iconSize_{iconSize} {
        }

        template<uint32_t SIZE>
        StaticMenuItem(char const * text, uint8_t const (&buffer)[SIZE], uint32_t payload = 0, void * payloadPtr = nullptr):
            MenuItem{payload, payloadPtr},
            text_{text}, 
            iconData_{buffer},
            iconSize_{SIZE} {
        }

        void text(std::string & text) const override { text = text_; }

        bool icon(Bitmap<ColorRGB> &bmp) const override {
            if (iconData_ == nullptr)
                return false;
            bmp.loadImage(PNG::fromBuffer(iconData_, iconSize_));
            return true;
        }

    private:
        char const * text_ = nullptr;
        uint8_t const * iconData_ = nullptr;
        uint32_t iconSize_ = 0;

    }; // rckid::StaticMenuItem


    /** Menu container API 

        The menu container is simply a wrapper around menu items vector.  
     */
    class Menu {
    public:

        Menu() = default;

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
            return *(items_[index]);
        }

        void add(MenuItem * item) {
            items_.push_back(item);
        }

        void clear() {
            for (auto mi : items_)
                delete mi;
            items_.clear();
        }

    private:
        std::vector<MenuItem *> items_;
    }; // rckid::Menu

} // namespace rckid

// 7a66c8
// 7a6680
// 7a6630
// 7a65d0


