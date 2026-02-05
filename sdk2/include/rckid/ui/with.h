#pragma once

#include <rckid/graphics/geometry.h>

namespace rckid::ui {

    /** Fluent widget manipulator. 
     
        Special helper class that exposes all common widget manipulation functions in a fluent way. The manipulator takes a pointer to widget and overrides operator << for operators. An operator is simply a functor that accepts a pointer to widget (or its subclass) and performs the desired modification on it. This allows for very compact, readable and extensible way of setting UI properties as further widgets just need to provide their own control operators. 
        
        Example usage:

            Panel * p = addChild(new Panel())
                << SetPosition(10, 10)
                << SetBg(Color::Blue())
            );

        The with helper can also be used later in the code to modify existing widgets:
        
            with(p) << SetVisible(false) << SetPosition(20, 20);

        For details about operator implementation, see the respctive widgets and their operators. Here is just the simplest contract:

            struct SomeOperator {
                uint32_t someParamater;
                SomeOperator(uint32_t someParameter): someParameter{someParameter} {}
            };
            template<typename T>
            inline with<T> operator << (with<T> w, SomeOperator so) {
                w->someMethod(so.someParameter);
                return w;
            }

        While this is slightly wordlier than simple method call with overloaded operator(), it allows unrealated widgets to share the same property setters.

     */
    template<typename T>
    class with {
    public:

        with(T * widget): widget_{widget} { }

        with(T & widget): widget_{ & widget } { }

        T * operator -> () const { return widget_; }

        operator T * () const { return widget_; }

    private:
        
        T * widget_;
    }; // ui::with<T>

}