#pragma once

#include <rckid/game/event.h>
#include <rckid/game/object.h>

namespace rckid::game {

    class Device : public Object {
    public:

        using ButtonStateChangedEvent = Event<Btn>;
        using GameLoopEvent = Event<>;

        ButtonStateChangedEvent onButtonPressed;

        EVENT_DESCRIPTOR(onButtonPressed, assets::icons_24::bookmark,
            "Fired when button is pressed",
            ARGS(
                ARG(with, Type::Button(), assets::icons_24::bookmark, "Newly pressed button"),
            ),
            CONNECT_WRAPPER((Object * obj, std::function<void(Value *)> handler) {
                obj->as<Device>()->onButtonPressed += [h = std::move(handler)](Btn) {
                    Value v;
                    h(& v);
                };
            })
        );

        ButtonStateChangedEvent onButtonReleased;

        EVENT_DESCRIPTOR(onButtonReleased, assets::icons_24::bookmark,
            "Fired when button is released",
            ARGS(
                ARG(with, Type::Button(), assets::icons_24::bookmark, "Newly released button"),
            ),
            CONNECT_WRAPPER((Object * obj, std::function<void(Value *)> handler) {
                obj->as<Device>()->onButtonReleased += [h = std::move(handler)](Btn) {
                    Value v;
                    h(& v);
                };
            })
        );

        GameLoopEvent onGameLoop;

        EVENT_DESCRIPTOR(onGameLoop, assets::icons_24::bookmark,
            "Event triggered every iteration of the game loop",
            ARGS(),
            CONNECT_WRAPPER((Object * obj, std::function<void(Value *)> handler) {
                obj->as<Device>()->onGameLoop += [h = std::move(handler)]() {
                    h(nullptr);
                };
            })
        );

        bool buttonDown(Btn btn) { return btnDown(btn); }

        METHOD_DESCRIPTOR(buttonDown, assets::icons_24::bookmark, 
            "Returns true if the given button is currently pressed (down)",
            Type::Boolean(),
            ARGS(
                ARG(by, Type::Button(), assets::icons_24::bookmark, "Which button"),
            ),
            CALL_WRAPPER((Object * obj, Value * args) {
                obj->as<Device>()->buttonDown(Btn::A);
                return Value{};
            })
        );

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

        Device(String name, Engine * engine):
            Object{std::move(name), engine} {
        }

    public:
        CLASS_DESCRIPTOR(Device, assets::icons_24::bookmark,
            "Device object",
            PARENT(Object),
            CAPABILITIES(
                .renderable = false,
                .constructible = false,
            ),
            METHODS(
                DESCRIPTOR(buttonDown)
            ),
            EVENTS(
                DESCRIPTOR(onButtonPressed),
                DESCRIPTOR(onButtonReleased),
                DESCRIPTOR(onGameLoop),
            )
        );

        ClassDescriptor const & typeDescriptor() const override { return descriptor; }

    }; 



} // namespace rckid::game