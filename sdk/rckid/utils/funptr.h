#pragma once

#include "../rckid.h"

namespace rckid {

    /** Wrapper around a function pointer with a void * argument. 
     
        Using std::function would be simpler, but it requires heap allocation and customizing it to support arenas would be too complex. 
     */
    template<typename T>
    class FunPtr {
        public:

            FunPtr() = default;

            FunPtr(T (*action)(void *), void * payload = nullptr): action_{action}, payload_{payload} {
            }

            bool valid() const { return action_ != nullptr; }

            T operator()() const {
                ASSERT(action_ != nullptr);
                return action_(payload_);
            }

        private:

            T (*action_)(void *);
            void * payload_;

        };


} // namespace rckid