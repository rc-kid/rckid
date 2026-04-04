#pragma once

#include <rckid/graphics/image_source.h>
#include <rckid/game/runtime.h>

#include <assets/icons_24.h>

#define ARGS(...) __VA_ARGS__
#define CALL_WRAPPER(...) __VA_ARGS__
#define EMIT_WRAPPER(...) __VA_ARGS__
#define CONNECT_WRAPPER(...) __VA_ARGS__
#define METHODS(...) __VA_ARGS__
#define EVENTS(...) __VA_ARGS__
#define DESCRIPTOR(NAME) & __ ## NAME
#define PARENT(NAME) & NAME :: descriptor
#define CAPABILITIES(...) { __VA_ARGS__ }

#define ARG(NAME, RET_TYPE, ICON, HELP) \
    ArgumentDescriptor{# NAME, RET_TYPE, ICON, HELP }

#define METHOD_DESCRIPTOR(NAME, ICON, HELP, RET_TYPE, METHOD_ARGS, WRAPPER_FOR_CALL) \
private: \
    static Value __ ## NAME ## _wrapper WRAPPER_FOR_CALL; \
    static constexpr ArgumentDescriptor __ ## NAME ## _args[] = { ArgumentDescriptor{}, METHOD_ARGS }; \
    static constexpr MethodDescriptor __ ## NAME { # NAME, RET_TYPE, ICON, HELP, __ ## NAME ## _args, __ ## NAME ## _wrapper }; \
public: 

// TODO
#define EVENT_DESCRIPTOR(NAME, ICON, HELP, EVENT_ARGS, WRAPPER_FOR_CONNECT) \
private: \
    static void __ ## NAME ## _wrapper WRAPPER_FOR_CONNECT; \
    static constexpr ArgumentDescriptor __ ## NAME ## _args[] = { ArgumentDescriptor{}, EVENT_ARGS }; \
    static constexpr EventDescriptor __ ## NAME { # NAME, ICON, HELP, __ ## NAME ## _args, __ ## NAME ## _wrapper }; \
public:


#define CLASS_DESCRIPTOR(NAME, ICON, HELP, PARENT, CAPABILITIES, METHODS, EVENTS) \
    private: \
        static constexpr MethodDescriptor const * const methods[] = { nullptr, METHODS }; \
        static constexpr EventDescriptor const * const events[] = { nullptr, EVENTS }; \
        static Value __create_wrapper(String name, Engine * engine) { \
            return Value{new NAME{std::move(name), engine}}; \
        } \
    public: \
        static constexpr ClassDescriptor descriptor{ \
            # NAME, ICON, HELP, \
            PARENT, \
            CAPABILITIES, \
            methods, \
            events, \
            __create_wrapper \
        }

namespace rckid::game {

    /** Descriptor base. 
     
        Descriptors are at the heart of the lower tiers of writing RCKid games. They wrap around C++ game engine objects and provide run-time type information and metadata about the classes with very little overhead. This information is then used by the visual event editor, blocks editor and the DSL code to build/typechedk the application. 

        All descriptors exist as constexpr objects so that they reside in the relatively abundant flash memory. They not only describe the objects, but also provide wrapper functions that map for the runtime world (Value and Type) to the underlying C++ objects and methods. This way, the runtime can call methods and connect to events. 
       
     */
    class Descriptor {
    public:
        using IconProvider = ImageSource (*)();

        char const * name() const { return name_; }
        char const * help() const { return help_; }

        // TODO get the icon
        //ImageSource icon() const { return iconSize_ == 0 ? iconProvider_(); }

    protected:
        constexpr Descriptor(char const * name, uint32_t iconSize, uint8_t const * iconBuffer, char const * help):
            name_{name},
            help_{help},
            iconSize_{iconSize},
            iconPtr_{iconBuffer} {
        }

    private:
        char const * const name_;
        char const * const help_;
        uint32_t const iconSize_;
        void const * const iconPtr_;
    }; // rckid::game::Descriptor

    /** Argument descriptor. 
     
        On top of default descriptor, an argument contains also its type
     */
    class ArgumentDescriptor : public Descriptor {
    public:

        template<size_t ICON_SIZE>
        constexpr ArgumentDescriptor(char const * name, Type type, uint8_t const (&iconBuffer)[ICON_SIZE], char const * help):
            Descriptor{name, ICON_SIZE, iconBuffer, help},
            type_{type} {
        }

        constexpr ArgumentDescriptor():
            Descriptor{nullptr, 0, nullptr, nullptr},
            type_{Type::Void()} {
        }

        Type type() const { return type_; }

    private:
        Type const type_;
    };

    /** Method descriptor
     
        On top of the default descriptor, contains the caller wrapper, and argument types and names. The caller wrapper is given object on which the method is to be invoked and a pointer to arguments represented by runtime Values. It should convert the values to the underlying types required by the call and then do the call.
     */
    class MethodDescriptor : public Descriptor {
    public:
        using CallWrapper = Value (*) (Object *, Value *);

        Type returnType() const { return returnType_; }

        uint32_t numArgs() const { return numArgs_; }
        
        Value call(Object * obj, Value * args) const { return callWrapper_(obj, args); }

        template<size_t ICON_SIZE, size_t ARGS_SIZE>
        constexpr MethodDescriptor(
            char const * name, 
            Type returnType, 
            uint8_t const (&iconBuffer)[ICON_SIZE], 
            char const * help, 
            ArgumentDescriptor const (&args)[ARGS_SIZE],
            CallWrapper callWrapper
        ):
            Descriptor{name, ICON_SIZE, iconBuffer, help},
            returnType_{returnType},
            numArgs_{ARGS_SIZE - 1},
            args_{args + 1},
            callWrapper_{callWrapper} {
        }

    private:

        Type const returnType_;
        uint32_t const numArgs_;
        ArgumentDescriptor const * const args_;
        CallWrapper const callWrapper_;
    };

    /** Event descriptor. 
     
        Events can have arguments like methods, but *not* return value (all events always return void and all event handlers return void as well).

        Connect wrapper is a function that takes object and std::function that takes Value * and registers with the event an std::function that converts the event arguments into Values and calls the runtime handler.

     */
    class EventDescriptor : public Descriptor {
    public:

        using ConnectWrapper = void (*)(Object * obj, std::function<void(Value*)>);

        void connect(Object * obj, std::function<void(Value*)> handler) {
            connectWrapper_(obj, std::move(handler));
        }

        template<size_t ICON_SIZE, size_t ARGS_SIZE>
        constexpr EventDescriptor(
            char const * name, 
            uint8_t const (&iconBuffer)[ICON_SIZE], 
            char const * help, 
            ArgumentDescriptor const (&args)[ARGS_SIZE],
            ConnectWrapper connectWrapper
        ):
            Descriptor{name, ICON_SIZE, iconBuffer, help},
            numArgs_{ARGS_SIZE - 1},
            args_{args + 1},
            connectWrapper_{connectWrapper} {
        }

    private:

        uint32_t const numArgs_;
        ArgumentDescriptor const * const args_;
        ConnectWrapper connectWrapper_;
    }; 

    // TODO change Capabilities in App.h to AppCapabilities
    class ObjectCapabilities {
    public:
        /** If true, the game object is renderable and its render method will be called.
         */
        bool renderable = true;
        /** True if the object can be created by user. 
         */
        bool constructible = true;
        /** Passive objects simply hold data and they do not have to be updated every loop iteration. This is typically true for assets.
         */
        bool passive = false;
    }; // rckid::game::ObjectCapabilities

    class ClassDescriptor : public Descriptor {
    public:

        using CreateWrapper = Value (*)(String name, Engine * engine);

        ObjectCapabilities const & capabilities() const { return capabilities_; }

        template<size_t ICON_SIZE, size_t METHODS_SIZE, size_t EVENTS_SIZE>
        constexpr ClassDescriptor(
            char const * name, 
            uint8_t const (&iconBuffer)[ICON_SIZE], 
            char const * help, 
            Descriptor const * parent,
            ObjectCapabilities capabilities,
            MethodDescriptor const * const (&methods)[METHODS_SIZE], 
            EventDescriptor const * const (&events)[EVENTS_SIZE],
            CreateWrapper createWrapper
        ):
            Descriptor{name, ICON_SIZE, iconBuffer, help},
            parent_{parent},
            capabilities_{capabilities},
            numMethods_{METHODS_SIZE - 1},
            methods_{methods + 1},
            numEvents_{EVENTS_SIZE - 1},
            events_{events + 1},
            createWrapper_{createWrapper} {
        }

        bool inheritsFromOrSame(ClassDescriptor const & other) const {
            ClassDescriptor const * current = this;
            do {
                if (current == &other)
                    return true;
                current = static_cast<ClassDescriptor const *>(current->parent_);
            } while (current != nullptr);
            return false;
        }
    
    private:
        Descriptor const * const parent_;
        ObjectCapabilities const capabilities_;
        uint32_t const numMethods_;
        MethodDescriptor const * const * const methods_;
        uint32_t const numEvents_;
        EventDescriptor const * const * const events_;
        CreateWrapper const createWrapper_;
    };

} // namespace rckid::game

