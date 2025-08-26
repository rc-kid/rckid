#pragma once

#include <vector>
#include <variant>

#include "../rckid.h"
#include "../utils/string.h"
#include "../utils/funptr.h"
#include "../graphics/bitmap.h"
#include "../graphics/png.h"

#include "../graphics/icon.h"

namespace rckid::ui {

    using Action = std::function<void()>;


    /** Menu templated by its payload type. The menu supports generators and menu items. 
     */
    template<typename PAYLOAD>
    class Menu {
    public:
        using MenuGenerator = std::function<ui::Menu<PAYLOAD>*()>;

        class MenuItem {
        public:
            String text;
            Icon icon;

            bool isGenerator() const { return std::holds_alternative<MenuGenerator>(payload_); }

            bool isAction() const { return std::holds_alternative<PAYLOAD>(payload_); }

            Menu * submenu() {
                ASSERT(isGenerator());
                return std::get<MenuGenerator>(payload_)();
            }

            MenuGenerator generator() {
                ASSERT(isGenerator());
                return std::get<MenuGenerator>(payload_);
            }

            PAYLOAD & action() {
                ASSERT(isAction());
                return std::get<PAYLOAD>(payload_);
            }

            MenuItem(String text, Icon icon, PAYLOAD payload):
                text{std::move(text)},
                icon{std::move(icon)},
                payload_{std::move(payload)} {
            }

            MenuItem(String text, Icon icon, MenuGenerator generator):
                text{std::move(text)},
                icon{std::move(icon)},
                payload_{std::move(generator)} {
            }

        private:
            std::variant<MenuGenerator, PAYLOAD> payload_;

        }; // Menu::Item

        class HistoryItem {
        public:
            HistoryItem(uint32_t index, MenuGenerator generator, HistoryItem * previous):
                index{index}, generator{generator}, previous{previous} {
            }

            uint32_t index;
            MenuGenerator generator;
            HistoryItem * previous;
        };

        static MenuItem Item(String name, Icon icon = Icon{}, PAYLOAD action = nullptr) {
            return MenuItem{std::move(name), std::move(icon), std::move(action) };
        }

        static MenuItem Generator(String name, Icon icon, MenuGenerator generator) {
            ASSERT(generator != nullptr);
            return MenuItem{std::move(name), std::move(icon), std::move(generator) };
        }

        Menu() = default;

        Menu(std::initializer_list<MenuItem> items):
            items_{std::move(items)} {
        }

        Menu(Menu && other) noexcept: items_{std::move(other.items_)} {
        }

        ~Menu() {
            items_.clear();
        }

        MenuItem const & operator [](uint32_t index) const {
            ASSERT(index < items_.size());
            return items_[index];
        }

        MenuItem & operator [](uint32_t index) {
            ASSERT(index < items_.size());
            return items_[index];
        }
        uint32_t size() const { return items_.size(); }
        
        void add(MenuItem item) {
            items_.push_back(std::move(item));
        }
        void clear() {
            items_.clear();
        }

        typename std::vector<MenuItem>::iterator begin() { return items_.begin(); }
        typename std::vector<MenuItem>::iterator end() { return items_.end(); }

    private:

        std::vector<MenuItem> items_;

    }; // rckid::ui::Menu


    /** Shorthand for the most common case where the menu paylod is an action. 
     */
    using ActionMenu = Menu<Action>;

} // namespace rckid::ui