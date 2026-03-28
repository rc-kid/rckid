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

        using ButtonStateChangedEvent = Event<>;
        using GameLoopEvent = Event<>;

        ButtonStateChangedEvent onUpPressed;
        ButtonStateChangedEvent onDownPressed;
        ButtonStateChangedEvent onLeftPressed;
        ButtonStateChangedEvent onRightPressed;
        ButtonStateChangedEvent onAPressed;
        ButtonStateChangedEvent onBPressed;
        ButtonStateChangedEvent onSelectPressed;
        ButtonStateChangedEvent onStartPressed;

        ButtonStateChangedEvent onUpReleased;
        ButtonStateChangedEvent onDownReleased;
        ButtonStateChangedEvent onLeftReleased;
        ButtonStateChangedEvent onRightReleased;
        ButtonStateChangedEvent onAReleased;
        ButtonStateChangedEvent onBReleased;
        ButtonStateChangedEvent onSelectReleased;
        ButtonStateChangedEvent onStartReleased;

        GameLoopEvent onGameLoop;

        bool buttonUpPressed() const { return btnDown(Btn::Up); }
        bool buttonDownPressed() const { return btnDown(Btn::Down); }
        bool buttonLeftPressed() const { return btnDown(Btn::Left); }
        bool buttonRightPressed() const { return btnDown(Btn::Right); }
        bool buttonAPressed() const { return btnDown(Btn::A); }
        bool buttonBPressed() const { return btnDown(Btn::B); }
        bool buttonSelectPressed() const { return btnDown(Btn::Select); }
        bool buttonStartPressed() const { return btnDown(Btn::Start); }

    protected:
        void loop() override {
            checkButton(Btn::Up, onUpPressed, onUpReleased);
            checkButton(Btn::Down, onDownPressed, onDownReleased);
            checkButton(Btn::Left, onLeftPressed, onLeftReleased);
            checkButton(Btn::Right, onRightPressed, onRightReleased);
            checkButton(Btn::A, onAPressed, onAReleased);
            checkButton(Btn::B, onBPressed, onBReleased);
            checkButton(Btn::Select, onSelectPressed, onSelectReleased);
            checkButton(Btn::Start, onStartPressed, onStartReleased);
            onGameLoop.emit();
        }

        void checkButton(Btn btn, ButtonStateChangedEvent & pressed, ButtonStateChangedEvent & released) {
            if (btnPressed(btn))
                pressed.emit();
            else if (btnReleased(btn))
                released.emit();
        }

    private:
        friend class Engine;

        Device(Engine * engine):
            Object{"Device", engine} {
        }

    }; 



} // namespace rckid::game