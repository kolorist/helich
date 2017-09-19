#ifndef __HELICH_MEMORY_MAP_H__
#define __HELICH_MEMORY_MAP_H__

#include <stdaliases.h>

template <class _AllocatorType>
struct MemoryRegion {
	typedef _AllocatorType*						AllocatorPtrType;

	const_cstr									Name;
	u32											SizeInBytes;
	
	AllocatorPtrType							AllocatorPtr;
};
#endif // __HELICH_MEMORY_MAP_H__
