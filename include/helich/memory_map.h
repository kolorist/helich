#pragma once

#include <floral.h>

#include "detail/alloc_region.h"

namespace helich {
	template <class t_allocator_type>
	struct memory_region {
		typedef t_allocator_type*				allocator_ptr_t;

		const_cstr								name;
		size									size_in_bytes;

		allocator_ptr_t							allocator_ptr;
	};

	struct memory_region_info {
		c8										name[512];
		size									size_in_bytes;
		voidptr									base_address;
		voidptr									allocator_ptr;
        dbginfo_extractor_func_t				dbg_info_extractor;
	};
}