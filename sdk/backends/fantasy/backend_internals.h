#pragma once

/** To model the memory management on the actual device we create a continuous array of device's memory size and set the bss end and stack limit pointers to it. We add extra room for the stack protection string which is written just above the heap beginning where on the device there is stack, while on the fantasy console there is nothing.
 */
#define RCKID_MEMORY_INITIALIZATION \
    char fantasyHeap[512 * 1024 + 5]; \
    char & __bss_end__ = *fantasyHeap; \
    char & __StackLimit = *(fantasyHeap + sizeof(fantasyHeap) - 5);
