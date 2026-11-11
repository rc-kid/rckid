#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <rckid/rckid.h>
#include <rckid/memory.h>

namespace rckid::dolang {

    /** Support for interning strings, parametrized by the pool.

        Wraps around an id that uniquely identifies the string in the pool. Interning is a bit complicated by the fact that we want 4 byte ids across fantasy and device platforms, while also being able to preserve non-heap strings, etc.

        Preservation of non-heap strings is not suppported on fantasy platforms where non heap pointers can exceed the 32bits. As a remedy non-heap strings are copied to the heap first, which slightly increases the memory usage.
     */
    template<typename POOL>
    class Interner {
    public:

        Interner(): Interner{String{}} { }

        Interner(String value) {
            auto i = values_.find(value);
            if (i == values_.end()) {
                if constexpr (sizeof(void*) != 4) {
                    if (! Heap::contains(value.c_str()))
                        value = String{value.c_str(), value.size() + 1}; // copy to heap if not already on heap
                }
                i = values_.insert(std::move(value)).first;
            }
            id_ = Heap::ptrToOffset(i->c_str());
        }

        char const * c_str() const {
            return Heap::offsetToPtr<char const>(id_);
        }

        /** Removes all interned strings. Any existing values of the pool are invalidated (their c_str() points to freed memory), so must only be called when no values of the pool exist. */
        static void clear() {
            // assigning an empty set releases the bucket array as well, which clear() would keep
            values_ = std::unordered_set<String>{};
        }

        bool operator == (Interner const & other) const { return id_ == other.id_; }
        bool operator != (Interner const & other) const { return id_ != other.id_; }

    private:

        friend class Token;

        Interner(uint32_t id): id_{id} { }

        uint32_t id_;

        // TODO this likely wastes memory and we should use dense set in the future. It's fine as static member because the initialization hell does not apply to us, tables are filled dynamically only when the apps requiring them start
        static inline std::unordered_set<String> values_;
    }; // rckid::dolang::Interner

    class Symbol : public Interner<Symbol> {
    public:
        using Interner<Symbol>::Interner;

        static void initialize() {
            clear();
            Symbol{"and"}; 
            Symbol{"as"}; 
            Symbol{"bool"}; 
            Symbol{"break"}; 
            Symbol{"clone"}; 
            Symbol{"const"}; 
            Symbol{"constructor"};
            Symbol{"continue"}; 
            Symbol{"do"}; 
            Symbol{"else"}; 
            Symbol{"enum"};
            Symbol{"event"}; 
            Symbol{"extends"}; 
            Symbol{"false"};
            Symbol{"for"};
            Symbol{"fun"}; 
            Symbol{"global"};
            Symbol{"if"}; 
            Symbol{"in"}; 
            Symbol{"is"}; 
            Symbol{"let"}; 
            Symbol{"match"};
            Symbol{"none"}; 
            Symbol{"not"}; 
            Symbol{"num"}; 
            Symbol{"optional"};
            Symbol{"or"};
            Symbol{"orelse"};
            Symbol{"return"}; 
            Symbol{"self"}; 
            Symbol{"step"}; 
            Symbol{"test"}; 
            Symbol{"text"};
            Symbol{"then"}; 
            Symbol{"true"}; 
            Symbol{"type"}; 
            Symbol{"use"}; 
            Symbol{"when"}; 
            Symbol{"while"}; 
            Symbol{"write"}; 
            Symbol{"xor"};
        }
    }; // rckid::dolang::Symbol

    static_assert(sizeof(Symbol) == 4, "Symbol should be 4 bytes");

    class TextLiteral : public Interner<TextLiteral> {
    public:
        using Interner<TextLiteral>::Interner;
    }; // rckid::dolang::TextLiteral

    static_assert(sizeof(TextLiteral) == 4, "TextLiteral should be 4 bytes");

} // namespace rckid::dolang
