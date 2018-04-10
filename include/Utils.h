#ifndef __HL_UTILS_H__
#define __HL_UTILS_H__

// self-provided headers
#include "macros.h"

// 3rd-party headers
#include <stdaliases.h>

namespace helich {

voidptr											align_address(voidptr i_addr, size i_alignment = HL_ALIGNMENT);

}

#endif // __HL_UTILS_H__
