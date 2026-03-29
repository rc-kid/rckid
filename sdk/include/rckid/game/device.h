#pragma once

#include <rckid/game/event.h>
#include <rckid/game/object.h>

namespace rckid::game {

    class Device : public Object {
    public:
        char const * className() const { return "Device"; }

        ObjectCapabilities capabilities() const override {
            return {
                .renderable = false,
                .constructible = false,
            };
        }

        using ButtonStateChangedEvent = Event<Btn>;
        using GameLoopEvent = Event<>;

        ButtonStateChangedEvent onButtonPressed;

        ButtonStateChangedEvent onButtonReleased;

        GameLoopEvent onGameLoop;

        bool buttonDown(Btn btn) { return btnDown(btn); }

    protected:
        void loop() override {
            checkButton(Btn::Up);
            checkButton(Btn::Down);
            checkButton(Btn::Left);
            checkButton(Btn::Right);
            checkButton(Btn::A);
            checkButton(Btn::B);
            checkButton(Btn::Select);
            checkButton(Btn::Start);
            onGameLoop.emit();
        }

        void checkButton(Btn btn) {
            if (btnPressed(btn))
                onButtonPressed.emit(btn);
            else if (btnReleased(btn))
                onButtonReleased.emit(btn);
        }

    private:
        friend class Engine;

        Device(Engine * engine):
            Object{"Device", engine} {
        }

    }; 



} // namespace rckid::game