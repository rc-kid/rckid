#pragma once

#include <rckid/string.h>
#include <rckid/game/object.h>

namespace rckid::game {

    /** Wraps game button. 
     
        Provides way of detecting state as well as press and release events. 
     */
    class Button : public Object {
    public:

        char const * className() const { return "Button"; }

        ObjectCapabilities capabilities() const override {
            return {
                .renderable = false,
                // TODO builtin? 
            };
        }

        using OnPressed = Event<>;
        using OnReleased = Event<>;

        bool down() const { return btnDown(btn_); }

        OnPressed onPressed;

        OnReleased onReleased;

    protected:

        void loop() override {
            if (btnPressed(btn_))
                onPressed.emit();
            else if (btnReleased(btn_))
                onReleased.emit();
        }

    private:

        friend class Engine;

        Button(String name, Btn btn):
            Object{std::move(name)},
            btn_{btn} {
        }

        Btn btn_;

    }; // rckid::game::Button

} // namespace rckid::game