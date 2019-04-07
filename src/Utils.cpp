// self-provided headers
#include "helich/Utils.h"

namespace helich {

voidptr align_address(voidptr i_addr, size i_alignment /* = HL_ALIGNMENT */)
{
    return (voidptr)((size)i_addr + (i_alignment - ((size)i_addr & (i_alignment - 1))));
}

}
