#pragma once

namespace rckid::ui {

    /** Application hedear. 
     
        The header displays information such as current time, battery status, sound volume, signal strenth, etc. 
     */
    class Header {
    public:

        /** Draws the header on the given graphics target. 
         
            The function is specialized below for the actual graphics targets it can write itself onto. 
         */
        template<typename TARGET>
        static void drawOn(TARGET & g);



    }; // rckid::ui::Header

} // namespace rckid::ui