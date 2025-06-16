#pragma once

#include <vector>

#include "../rckid.h"
#include "../utils/string.h"
#include "../utils/funptr.h"
#include "../graphics/bitmap.h"
#include "../graphics/png.h"

#include "../graphics/icon.h"

namespace rckid::ui {


    class Menu {
    public:

        class Item {
        public:
            String text;
            Icon icon;

            explicit Item(String text):
                text{std::move(text)} {
            }

            Item(String text, Icon icon):
                text{std::move(text)}, 
                icon{std::move(icon)} {
            }

            virtual ~Item() = default;
        }; // rckid::Menu::Item

        using Generator = std::function<ui::Menu *()>;
        using Action = std::function<void()>; 

        Menu() = default;

        Menu(std::initializer_list<Item *> items):
            items_{items} {
        }

        Menu(Menu && other) noexcept: items_{std::move(other.items_)} {
        }

        ~Menu() {
            clear();
        }

        Item const & operator [](uint32_t index) const {
            ASSERT(index < items_.size());
            return *items_[index];
        }

        Item & operator [](uint32_t index) {
            ASSERT(index < items_.size());
            return *items_[index];
        }
        uint32_t size() const { return items_.size(); }
        
        void add(Item * item) {
            items_.push_back(item);
        }
        void clear() {
            for (auto & mi : items_)
                delete mi;
            items_.clear();
        }

        std::vector<Item *>::iterator begin() { return items_.begin(); }
        std::vector<Item *>::iterator end() { return items_.end(); }

    private:
        std::vector<Item*> items_;

    }; // rckid::Menu

} // namespace rckid::ui