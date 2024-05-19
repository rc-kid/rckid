#pragma once

#include "rckid/rckid.h"

namespace rckid {

    class Menu;

    /** Menu generator function. 
     
        Takes no arguments and returns a heap allocated menu. The function is used for dynamic generation of submenus in multi-layer menu systems. 
     */
    typedef Menu * (*MenuGenerator)();

    /** Menu item, designed for minimal overhead. 
     
        Note that the menu item does *not* own its data (i.e. the icon or the text). This is fine for static purposes where the label and the icon are part of code, but will break when using dynamic data
     */
    class MenuItem {
    public:

        char const * text; 
        uint8_t const * icon;
        size_t const iconBytes;

        MenuGenerator submenuGenerator() const {
            return isSubmenu_ ? reinterpret_cast<MenuGenerator>(payload_) : nullptr;
        }

        void * payload() const { 
            return isSubmenu_ ? nullptr : payload_;
        }
 
        MenuItem(char const * text, uint8_t const * icon, size_t iconBytes, void * payload = nullptr):
            text{text}, icon{icon}, iconBytes{iconBytes}, payload_{payload}, isSubmenu_{false} {}

        MenuItem(char const * text, uint8_t const * icon, size_t iconBytes, MenuGenerator submenu):
            text{text}, icon{icon}, iconBytes{iconBytes}, payload_{reinterpret_cast<void*>(submenu)}, isSubmenu_{true} {}

        template<int SIZE, typename T>
        static MenuItem * create(char const * text, uint8_t const (& icon)[SIZE], T * payload) {
            return new MenuItem{text, icon, SIZE, reinterpret_cast<void*>(payload)};
        }

        template<int SIZE>
        static MenuItem * create(char const * text, uint8_t const (& icon)[SIZE]) {
            return new MenuItem{text, icon, SIZE};
        }

        template<int SIZE>
        static MenuItem * createSubmenu(char const * text, uint8_t const (& icon)[SIZE], MenuGenerator submenu) {
            return new MenuItem{text, icon, SIZE, submenu};
        }

    private:

        bool const isSubmenu_;
        void * const payload_;

    }; 

    /** Menu is simply a collection of menu items. 
     
        NOTE this could be a std::vector of them, but having own class gives some leeway if we need more from the menus, such as dynamically resizing menus, or dynamic item generation, etc. 
     */
    class Menu {
    public:

        Menu() = default;
        Menu(std::initializer_list<MenuItem*> items): items_{std::move(items)} {}
        virtual ~Menu() { for (auto i : items_) delete i; }

        size_t size() const { return items_.size(); }

        MenuItem const & operator [] (size_t index) const { return *items_[index]; }
        MenuItem & operator [] (size_t index) { return *items_[index]; }

        MenuItem const & at(size_t index) const { return *items_[index]; }
        MenuItem & at(size_t index) { return *items_[index]; }

    private:
        std::vector<MenuItem*> items_;
    }; 

    /** Defines a stack of menu hierarchies, displayed one on top of another. 
     
        The menu stack provides access to the menu layers, adding and removing layers and menu compression (whereas the submenu levels are all deleted as their menus can be recreated from the root level and menu generator functions).
        
        NOTE: This is just a base class defining the API, use the children, such as StaticMenuStack for actual functionality. 
     */
    class MenuStack {
    public:
        struct Level {
             Menu * menu = nullptr;
            size_t index = 0;
        }; 

        virtual Level & at(size_t index) = 0;
        virtual Level & push(Level const & l) = 0;
        /** Pops the current top of the menu stack and returns the new stack top. 
         */
        virtual Level & pop() = 0;

        size_t depth() const {
            return depth_;
        }

        Level & top() { return at(depth_); }

        void reconstruct() {
            for (size_t i = 1; i <= depth_; ++i) {
                Level & parent = at(i-1);
                at(i).menu = parent.menu->at(parent.index).submenuGenerator()();
            }
        }

        void compress() {
            for (size_t i = depth_; i > 0; --i) {
                Level & l = at(i);
                delete l.menu;
                l.menu = nullptr;
            }
        }

    protected:
        size_t depth_ = 0;


    }; // MenuStack

    /** Statically sized menu stack. 
     
        Useful for cross heap arena operation as it allocates no heap space if compression is used. 
     */
    template<size_t DEPTH = 8>
    class StaticMenuStack :  public MenuStack {
    public:
        StaticMenuStack(Menu * menu) {
            levels_[0].menu = menu;
            levels_[0].index = 0;
        }

        Level & at(size_t index) override {
            ASSERT(index < DEPTH);
            return levels_[index];
        }

        Level & push(Level const & l) override {
            ASSERT(depth_ < DEPTH - 1);
            ++depth_;
            levels_[depth_] = l;
            return levels_[depth_];
        }
        
        Level & pop() override {
            ASSERT(depth_ > 0);
            delete levels_[depth_].menu;
            --depth_;
            return levels_[depth_];
        }

    private:
        Level levels_[DEPTH];

    }; // rckid::StaticMenuStack

}