#pragma once

#include <utility>

#include "../error.h"

namespace rckid {

    /** Wrapper around a function pointer with a void * argument. 
     
        Using std::function would be simpler, but it requires heap allocation and customizing it to support arenas would be too complex. 
     */
    template<typename Ret, typename... Args>
    class FunPtr {
    public:
        FunPtr() = default;
    
        FunPtr(Ret (*action)(void*, Args...), void* payload = nullptr)
            : action_{action}, payload_{payload} {}
    
        bool valid() const { return action_ != nullptr; }
    
        Ret operator()(Args... args) const {
            ASSERT(action_ != nullptr);
            return action_(payload_, std::forward<Args>(args)...);
        }
    
    private:
        Ret (*action_)(void*, Args...);
        void* payload_;
    };    


} // namespace rckid