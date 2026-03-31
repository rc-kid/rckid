#pragma once

namespace rckid::game {

    /** Game event
     */
    template<typename... ARGS>
    class Event {
    public:

        using Handler = std::function<void(ARGS...)>;

        Event() = default;

        Event(Event const &) = delete;

        ~Event() {
            clear();
        }

        /** Adds given handler to the event. 
         
            When event is emited, all handlers will be executed in the order they were added.
         */
        Event & operator += (Handler cb) {
            if (! cb_) {
                cb_ = std::move(cb);
            } else {
                Event * e = this;
                while (e->next_ != nullptr)
                    e = e->next_;
                e->next_ = new Event{std::move(cb)};
            }
            return *this;
        }

        /** Clears the event by unregistering any active handlers. 
         */
        void clear() {
            cb_ = nullptr;
            Event * e = next_;
            while (e != nullptr) {
                Event * x = e;
                e = e->next_;
                // break the link & single delete
                x->next_ = nullptr;
                delete x;
            }
            next_ = nullptr;
        }

        /** Returns true if the event is connected, i.e. if any handlers have been added to it. 
         */
        bool connected() const { return cb_ ? true : false; }

        /** Emits the event, calling the handlers in order they were added to the event. 
         */
        void emit(ARGS... args) {
            if (! cb_)
                return;
            cb_(args...);
            Event * e = next_;
            while (e != nullptr) {
                e->cb_(args...);
                e = e->next_;
            }
        }
        
    private:

        Event(Handler cb): cb_{std::move(cb)} {}

        Handler cb_;
        Event * next_ = nullptr;

    }; // rckid::game::Event


} // namespace rckid::game