#pragma once

#include <vector>

#include <rckid/rckid.h>
#include <rckid/string.h>
#include <rckid/memory.h>
#include <rckid/graphics/bitmap.h>
#include <rckid/ui/image.h>

#include <assets/icons_24.h>

namespace rckid::ui {

    class Label;
    class Menu;

    class MenuItem {
    public:
        using ActionEvent = std::function<void()>;
        using GeneratorEvent = std::function<unique_ptr<Menu>()>;
        using DecoratorEvent = std::function<void(MenuItem &, Image *, Label *)>;

        String text;
        ImageSource icon;
        uint32_t payload = 0;

        bool isAction() const { return isAction_; }

        bool isGenerator() const { return ! isAction_; }

        MenuItem(String text, ImageSource icon, ActionEvent action):
            text{std::move(text)}, icon{std::move(icon)}, isAction_{true}, action_{std::move(action)} {
        }

        MenuItem(String text, ActionEvent action):
            MenuItem{std::move(text), ImageSource{}, std::move(action)} {
        }

        MenuItem(MenuItem && from) noexcept :
            text{std::move(from.text)}, icon{std::move(from.icon)}, payload{from.payload}, isAction_{from.isAction_}, decorator_{std::move(from.decorator_)} 
        {
            if (isAction_)
                new (&action_) ActionEvent(std::move(from.action_));
            else
                new (&generator_) GeneratorEvent(std::move(from.generator_));
        }

        MenuItem & operator = (MenuItem && other) {
            if (& other != this) {
                text = std::move(other.text);
                icon = std::move(other.icon);
                payload = other.payload;
                isAction_ = other.isAction_;
                decorator_ = std::move(other.decorator_);
                if (isAction_)
                    action_ = std::move(other.action_);
                else
                    generator_ = std::move(other.generator_);
            }
            return *this;
        }

        static MenuItem Generator(String text, ImageSource icon, GeneratorEvent generator) {
            return MenuItem{std::move(text), std::move(icon), std::move(generator)};
        }

        static MenuItem Generator(String text, GeneratorEvent generator) {
            return MenuItem{std::move(text), ImageSource{}, std::move(generator)};
        }

        MenuItem withPayload(uint32_t payload) && {
            this->payload = payload;
            return std::move(*this);
        }

        MenuItem withDecorator(DecoratorEvent decorator) && {
            this->decorator_ = std::move(decorator);
            return std::move(*this);
        }

        /** Shorthand function for creating a menu item with check decorator based on a boolean condition.
         */
        MenuItem withCheckDecorator(std::function<bool()> isChecked) && {
            this->decorator_ = [isChecked = std::move(isChecked)](MenuItem &, Image * icon, Label *) {
                if (isChecked())
                    icon->addChild(new ui::Image{})
                        << SetRect(Rect::XYWH(40, 40, 24, 24))
                        << SetBitmap(assets::icons_24::checked);
            };
            return std::move(*this);
        }

        /** Shorthand function for menu item that show toogle on/off icon, i.e. an icon even when off.
         */
        MenuItem withToggleDecorator(std::function<bool()> isEnabled) && {
            this->decorator_ = [isEnabled = std::move(isEnabled)](MenuItem &, Image * icon, Label *) {
                icon->addChild(new ui::Image{})
                    << SetRect(Rect::XYWH(40, 40, 24, 24))
                    << SetBitmap(isEnabled() ? ImageSource{assets::icons_24::switch_on} : ImageSource{assets::icons_24::switch_off});
            };
            return std::move(*this);
        }

        ~MenuItem() {
            if (isAction())
                action_.~ActionEvent();
            else
                generator_.~GeneratorEvent();
        }

        ActionEvent const & action() const{
            ASSERT(isAction());
            return action_;
        }

        GeneratorEvent const & generator() const {
            ASSERT(! isAction());
            return generator_;
        }

        DecoratorEvent const & decorator() const{
            return decorator_;
        }

    private:

        MenuItem(String text, ImageSource icon, GeneratorEvent generator):
            text{std::move(text)}, icon{std::move(icon)}, isAction_{false}, generator_{std::move(generator)} {
        }

        bool isAction_ = true;

        union {
            GeneratorEvent generator_;
            ActionEvent action_;
        }; 

        DecoratorEvent decorator_;
    }; // rckid::ui::MenuItem

    class Menu {
    public:

        using iterator = std::vector<MenuItem>::iterator;
        using const_iterator = std::vector<MenuItem>::const_iterator;
        using reference = std::vector<MenuItem>::reference;

        virtual ~Menu() = default;

        bool empty() const { return items_.empty(); }

        uint32_t size() const { return items_.size(); }

        void reserve(uint32_t size) { items_.reserve(size); }

        const_iterator begin() const { return items_.begin(); }
        const_iterator end() const { return items_.end(); }
        iterator begin() { return items_.begin(); }
        iterator end() { return items_.end(); }

        void push_back(MenuItem item) { items_.push_back(std::move(item)); }

        template< class... Args >
        reference emplace_back( Args&&... args ) {
            return items_.emplace_back(std::forward<Args>(args)...);
        }

        iterator insert(iterator pos, MenuItem item) { return items_.insert(pos, std::move(item)); }

        iterator erase(iterator pos) { return items_.erase(pos); }

        MenuItem & at(uint32_t index) { return items_[index]; }

        class Context {
        public:
            bool empty() const { return generator_ == nullptr; }

            Context * parent() const { return parent_.get(); }

            bool populated() const { return menu_ != nullptr; }

            Menu * menu() const { return menu_.get(); }
            uint32_t index() const { return index_; }

            MenuItem const * currentItem() const { 
                ASSERT(populated());
                if (index_ >= menu_->size())
                    return nullptr;
                return & menu_->at(index_);
            }

            void openNew(MenuItem::GeneratorEvent generator) {
                if (! empty()) {
                    auto newParent = std::make_unique<Context>(std::move(*this));
                    parent_ = std::move(newParent);
                }
                generator_ = std::move(generator);
                menu_ = nullptr;
                index_ = 0;
            }

            void pop() {
                ASSERT(parent_ != nullptr);
                *this = std::move(*parent_);
            }

            void setIndex(uint32_t index) {
                ASSERT(! empty());
                index_ = index;
            }

            void releaseResources() {
                menu_.reset();
                if (parent_ != nullptr)
                    parent_->releaseResources();
            }

            void populate() {
                ASSERT(menu_ == nullptr);
                if (parent_!= nullptr && ! parent_->populated())
                    parent_->populate();
                menu_ = generator_();
            }

        private:

            MenuItem::GeneratorEvent generator_;
            std::unique_ptr<Menu> menu_;
            uint32_t index_ = 0;
            std::unique_ptr<Context> parent_;
        }; // Menu::Context


    private:
        std::vector<MenuItem> items_;
    }; 

    inline Menu & operator << (Menu & menu, MenuItem item) {
        menu.push_back(std::move(item));
        return menu;
    }
} // rckid::ui