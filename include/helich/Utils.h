#pragma once

// self-provided headers
#include "macros.h"

// 3rd-party headers
#include <floral/stdaliases.h>

namespace helich {

voidptr											align_address(voidptr i_addr, size i_alignment = HL_ALIGNMENT);

}