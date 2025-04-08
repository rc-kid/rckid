#pragma once

#include <vector>

#include "../rckid.h"
#include "../graphics/bitmap.h"
#include "../utils/string.h"

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
            static constexpr uint32_t KIND = 0;

            Item(String text): text_{std::move(text)} {
            }

            Item(String text, uint8_t const * icon, uint32_t iconSize): text_{std::move(text)}, icon_{icon}, iconSize_{iconSize} {
            }

            virtual ~Item() = default;

            virtual uint32_t kind() const { return KIND; }

            template<typename T>
            bool is() const {
                return kind() == T::KIND;
            }

            template<typename T>
            T * as() {
                if (! is<T>())
                    return nullptr;
                return static_cast<T*>(this);
            }

            String const & text() const { return text_; }

            /*
            void fillText(std::string & text) const {
                text = text_;
            }
            */

            Bitmap<16> icon() const {
                return Bitmap<16>{PNG::fromBuffer(icon_, iconSize_)};
            }

        private:

            String text_;
            uint8_t const * icon_ = nullptr;
            uint32_t iconSize_ = 0;

        }; // rckid::ui::Menu::Item

        /** Item with associated action, which is a simple std::function for simplicity.
         */
        class ActionItem : public Item {
        public:

            ActionItem(String text, std::function<void()> action): Item{std::move(text)}, action_{action} {
            }

            ActionItem(String text, uint8_t const * icon, uint32_t iconSize, std::function<void()> action): Item{std::move(text), icon, iconSize}, action_{action} {
            }

            template<uint32_t SIZE>
            ActionItem(String text, uint8_t const (&buffer)[SIZE], std::function<void()> action): Item{std::move(text), buffer, SIZE}, action_{action} {
            }

            static constexpr uint32_t KIND = 1;
            uint32_t kind() const override { return KIND; }

            std::function<void()> const & action() const { return action_; }
            void setAction(std::function<void()> const & action) { action_ = action; }

        private:
            std::function<void()> action_;
        }; // rckid::ui::Menu::ActionItem

        /** Item with associated submenu generator. 
         */
        class SubmenuItem : public Item {
        public:
            static constexpr uint32_t KIND = 2;
            uint32_t kind() const override { return KIND; }

            std::function<Menu *()> const & generator() const { return generator_; }
            void setGenerator(std::function<Menu *()> const & generator) { generator_ = generator; }

        private:
            std::function<Menu *()> generator_;
        }; // rckid::ui::Menu::SubmenuItem

        Menu() = default;

        Menu(std::initializer_list<Item *> items):
            items_{items} {
        }

        Menu(Menu && other) noexcept: items_{std::move(other.items_)} {
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

        Item const & operator [] (uint32_t index) const {
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