#pragma once

#include <vector>

#include <rckid/rckid.h>
#include <rckid/string.h>
#include <rckid/memory.h>
#include <rckid/graphics/bitmap.h>
#include <rckid/ui/image.h>

namespace rckid::ui {

    class Label;

    class MenuItem {
    public:
        using ActionEvent = std::function<void()>;
        using GeneratorEvent = std::function<unique_ptr<std::vector<MenuItem>>()>;
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
            text{std::move(from.text)}, icon{std::move(from.icon)}, payload{from.payload}, isAction_{from.isAction_} 
        {
            if (isAction_)
                new (&action_) ActionEvent(std::move(from.action_));
            else
                new (&generator_) GeneratorEvent(std::move(from.generator_));
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

        ~MenuItem() {
            if (isAction())
                action_.~ActionEvent();
            else
                generator_.~GeneratorEvent();
        }

        ActionEvent & action() {
            ASSERT(isAction());
            return action_;
        }

        GeneratorEvent & generator() {
            ASSERT(! isAction());
            return generator_;
        }

        DecoratorEvent & decorator() {
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

    using Menu = std::vector<MenuItem>;

    inline Menu & operator << (Menu & menu, MenuItem item) {
        menu.push_back(std::move(item));
        return menu;
    }
} // rckid::ui