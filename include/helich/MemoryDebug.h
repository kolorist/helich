#pragma once

#include <floral/stdaliases.h>

namespace helich {
    struct debug_memory_block {
        p8										frame_address;
        size                                    frame_size;
        bool                                    is_allocated;
    };
}