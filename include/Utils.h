#ifndef __HL_UTILS_H__
#define __HL_UTILS_H__

// self-provided headers
#include "macros.h"

// 3rd-party headers
#include <stdaliases.h>

namespace helich {

voidptr											alignAddress(voidptr addr, u32 alignment = HL_ALIGNMENT);

}

#endif // __HL_UTILS_H__
