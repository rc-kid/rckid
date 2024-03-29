#pragma once

#include "rckid.h"

namespace rckid {

    /** A simple timer utility with [us] precision. 
     
        The timer starts when initialized, total and lap times can be retrieved. Particularly useful for measuring various performance metrics. 
    */
    class Timer {
    public:
        Timer():
            start_{uptimeUs()},
            lapStart_{start_} {
        }

        /** Returns total time in [us]. */
        unsigned total() const { return uptimeUs() - start_; }

        /** Returns current lap time in [us]. */
        unsigned lap() const { return uptimeUs() - lapStart_; }

        /** Retrurns the length of current lap and starts a new one in [us]. */
        unsigned newLap() {
            unsigned t = uptimeUs();
            unsigned result = t - lapStart_;
            lapStart_ = t;
            return result;
        }

    private:
        
        unsigned start_;
        unsigned lapStart_;
    }; 

} // namespace rckid