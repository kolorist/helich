#ifndef __HL_MEMORY_DEBUG_H__
#define __HL_MEMORY_DEBUG_H__

#include <stdaliases.h>

namespace helich {
    struct debug_memory_block {
        p8										frame_address;
        size                                    frame_size;
        bool                                    is_allocated;
    };
}

#endif // __HL_MEMORY_DEBUG_H__
