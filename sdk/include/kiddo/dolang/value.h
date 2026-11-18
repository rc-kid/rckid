#pragma once

#include <kiddo/dolang/num.h>


namespace kiddo::dolang {

    /** Any doLang value.
     
        Each value is 8 bytes long, which is just enough to store rational number. As a tag for the union we use the fact that denomninator is always positive, so the sign bit can be used to indicate the type. The type is important for the garbage collector, not the interpreter, since dolang is statically typed and therefore all values types are known at compile time. 
        
        The values themselves are used in the register stack used by the VM.
     */
    class Value {
    public:
      union {
        Num num_;
      };
    }; // dolang::value

    static_assert(sizeof(Value) == 8);

} // namespace kiddo::dolang