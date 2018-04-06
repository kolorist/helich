#ifndef __HELICH_MEMORY_MAP_H__
#define __HELICH_MEMORY_MAP_H__

#include <floral.h>

#include "detail/AllocRegion.h"

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
        alloc_region_dgbinfo_extractor			dbg_info_extractor;
	};
}

#endif // __HELICH_MEMORY_MAP_H__
