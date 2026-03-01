#pragma once

#include <rckid/contacts.h>

namespace rckid {

    /** Returns the current remaining budget of the device */
    uint32_t remainingBudget();

    /** Returns true of the device is running in parent mode. 
     
        Parent mode is required for advanced pim functions, such as budget allowance settings, etc.
     */
    bool parentMode(); 




} // namespace rckid