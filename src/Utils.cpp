// self-provided headers
#include "Utils.h"

namespace helich {

voidptr alignAddress(voidptr addr, u32 alignment /* = HL_ALIGNMENT */)
{
    return (void*)((u32)addr + (alignment - ((u32)addr & (alignment - 1))));    
}

}