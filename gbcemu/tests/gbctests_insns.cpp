
#include "gbctests.h"

namespace rckid::gbcemu {

    /** STOP instruction 
        
        The tests expect the stop instruction takes only one byte, which is arguably correct, because according to the manual, it takes one byte, but the instruction byte immediately after it should always be nop as it is discarded during wakeup. Thus in the emulator, we always advance by 2 to simulate the throwaway byte. 
     */
    //#include "insns/10.inc.h"

    /** DAA instruction
     
        A complex one with little documentation about corner cases with invalid inputs. At the moment not worth the time investment. 
     */
    //#include "insns/27.inc.h"

    /** EI instruction
     
        This is a cycle issue as the interrupt enable flag is not enabled immediately after the EI instruction, but after the *next* one. This is ignored by the emulator for now. 
     */
    //#include "insns/fb.inc.h" 
}
