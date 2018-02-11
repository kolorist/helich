#ifndef __HL_MEMORY_DEBUG_H__
#define __HL_MEMORY_DEBUG_H__

#include <stdaliases.h>

namespace helich {
    struct DebugMemBlock {
        s8*                                     pm_FrameAddress;
        u32                                     pm_FrameSize;
        bool                                    pm_IsAllocated;
    };
}

#endif // __HL_MEMORY_DEBUG_H__
