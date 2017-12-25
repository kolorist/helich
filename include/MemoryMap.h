#ifndef __HELICH_MEMORY_MAP_H__
#define __HELICH_MEMORY_MAP_H__

#include <floral.h>
namespace helich {
	template <class _AllocatorType>
	struct MemoryRegion {
		typedef _AllocatorType*					AllocatorPtrType;

		const_cstr								Name;
		u32										SizeInBytes;

		AllocatorPtrType						AllocatorPtr;
	};

	struct MemoryRegionInfo {
		c8										Name[512];
		u32										SizeInBytes;
		voidptr									BaseAddress;
	};
}

#endif // __HELICH_MEMORY_MAP_H__
