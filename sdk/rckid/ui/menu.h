#pragma once

#include <vector>

#include "../rckid.h"
#include "../utils/string.h"
#include "../utils/funptr.h"
#include "../graphics/bitmap.h"
#include "../graphics/png.h"

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

            Item(String text, uint8_t const * icon, uint32_t iconSize): text_{std::move(text)}, icon_{icon}, iconSize_{iconSize} { }

            template<uint32_t SIZE>
            Item(String text, uint8_t const (&buffer)[SIZE]): Item(text, buffer, SIZE) { }

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

            Bitmap<ColorRGB> icon() const {
                // when decoding the icon, create new arena to avoid fragmentation as the PNG buffer will be disable immediately after the decoding takes place
                NewArenaGuard g{};
                return Bitmap<ColorRGB>{ARENA(PNG::fromBuffer(icon_, iconSize_))};
            }

        private:

            String text_;
            uint8_t const * icon_ = nullptr;
            uint32_t iconSize_ = 0;

        }; // rckid::ui::Menu::Item

        using Action = FunPtr<void>;

        /** Item with associated action, which is a simple std::function for simplicity.
         */
        class ActionItem : public Item {
        public:

            ActionItem(String text, Action action): 
                Item{std::move(text)}, 
                action_{std::move(action)} {
            }

            ActionItem(String text, uint8_t const * icon, uint32_t iconSize, Action action):
                Item{std::move(text), icon, iconSize}, 
                action_{std::move(action)} {
            }

            template<uint32_t SIZE>
            ActionItem(String text, uint8_t const (&buffer)[SIZE], Action action):
                Item{std::move(text), buffer, SIZE}, 
                action_{std::move(action)} {
            }

            static constexpr uint32_t KIND = 1;
            uint32_t kind() const override { return KIND; }

            Action const & action() const { return action_; }
            void setAction(Action action) { action_ = std::move(action); }

        private:
            Action action_;
            void * actionPayload_;
        }; // rckid::ui::Menu::ActionItem

        using Generator = FunPtr<Menu*>;
        /** Item with associated submenu generator. 
         */
        class SubmenuItem : public Item {
        public:

            SubmenuItem(String text, Generator generator): 
                Item{std::move(text)}, 
                generator_{std::move(generator)} {
            }

            SubmenuItem(String text, uint8_t const * icon, uint32_t iconSize, Generator generator): 
                Item{std::move(text), icon, iconSize}, 
                generator_{std::move(generator)} {
            }

            template<uint32_t SIZE>
            SubmenuItem(String text, uint8_t const (&buffer)[SIZE], Generator generator):
                Item{std::move(text), buffer, SIZE}, 
                generator_{std::move(generator)} {
            }

            static constexpr uint32_t KIND = 2;
            uint32_t kind() const override { return KIND; }

            Generator const & generator() const { return generator_; } 

            void setGenerator(Generator generator) { 
                generator_ = std::move(generator);
             }

        private:
            Generator generator_;
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