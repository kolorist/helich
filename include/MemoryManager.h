#ifndef __HELICH_MEMORY_MANAGER_H__
#define __HELICH_MEMORY_MANAGER_H__

#include <floral.h>

#include "macros.h"
//#include "IMemoryMappable.h"
#include "MemoryMap.h"
// TODO: this many headers?
#include "Allocator.h"
#include "AllocSchemes.h"
#include "TrackingPolicies.h"

namespace helich {

	extern FixedAllocator<PoolScheme, sizeof(DebugEntry), NoTrackingPolicy> g_TrackingAllocator;
#define MEMORY_TRACKING_SIZE					SIZE_MB(32)
#define MAX_MEM_REGIONS							32

class MemoryManager {
public:
	MemoryManager();
	~MemoryManager();

	const voidptr								AllocateGlobalMemory(voidptr baseAddress, const u32 sizeInBytes);

	template <class ... _AllocatorRegions>
	const void Initialize(_AllocatorRegions ... regions) {
		if (!m_BaseAddress) {
			u32 totalSize = InternalComputeMem(regions...);

			// this call will update m_BaseAddress
			AllocateGlobalMemory((voidptr)0x20000000, totalSize);
			pm_MemRegionsNumber = 0;

			InternalInit(m_BaseAddress, 
				regions...);
		}
	}

private:
	template <class _AllocatorType>
	const u32 InternalComputeMem(MemoryRegion<_AllocatorType> al)
	{
		return al.SizeInBytes + MEMORY_TRACKING_SIZE;
	}

	template <class _AllocatorTypeHead, class ... _AllocatorTypeRests>
	const u32 InternalComputeMem(MemoryRegion<_AllocatorTypeHead> headAl,
		MemoryRegion<_AllocatorTypeRests> ... restAl)
	{
		return (headAl.SizeInBytes + InternalComputeMem(restAl...));
	}

	template <class _AllocatorType>
	const bool InternalInitTracking(voidptr baseAddress, 
		MemoryRegion<_AllocatorType> al) 
	{
		((_AllocatorType*)(al.AllocatorPtr))->MapTo(baseAddress, al.SizeInBytes, al.Name);
		return true;
	}

	// end of recursion
	template <class _AllocatorType>
	const bool InternalInit(voidptr baseAddress, 
		MemoryRegion<_AllocatorType> al) 
	{
		((_AllocatorType*)(al.AllocatorPtr))->MapTo(baseAddress, al.SizeInBytes, al.Name);

		strcpy(pm_MemRegions[pm_MemRegionsNumber].Name, al.Name);
		pm_MemRegions[pm_MemRegionsNumber].SizeInBytes = al.SizeInBytes;
		pm_MemRegions[pm_MemRegionsNumber].BaseAddress = baseAddress;
		pm_TotalMemInBytes += al.SizeInBytes;
		pm_MemRegionsNumber++;

		// last one, tracking debug info pool
		s8* nextBase = (s8*)baseAddress + al.SizeInBytes;
		InternalInitTracking(nextBase,
			MemoryRegion<FixedAllocator<PoolScheme, sizeof(DebugEntry), NoTrackingPolicy>> { "helich/tracking", MEMORY_TRACKING_SIZE, &g_TrackingAllocator });

		strcpy(pm_MemRegions[pm_MemRegionsNumber].Name, "helich/tracking");
		pm_MemRegions[pm_MemRegionsNumber].SizeInBytes = MEMORY_TRACKING_SIZE;
		pm_MemRegions[pm_MemRegionsNumber].BaseAddress = nextBase;
		pm_TotalMemInBytes += MEMORY_TRACKING_SIZE;
		pm_MemRegionsNumber++;
		return true;
	}

	// compile-time recursive initialization
	template <class _AllocatorTypeHead, class ... _AllocatorTypeRests>
	const bool InternalInit(voidptr baseAddress, 
		MemoryRegion<_AllocatorTypeHead> headAl, 
		MemoryRegion<_AllocatorTypeRests> ... restAl)
	{
		// init here
		((_AllocatorTypeHead*)(headAl.AllocatorPtr))->MapTo(baseAddress, headAl.SizeInBytes, headAl.Name);
		s8* nextBase = (s8*)baseAddress + headAl.SizeInBytes;
		
		strcpy(pm_MemRegions[pm_MemRegionsNumber].Name, headAl.Name);
		pm_MemRegions[pm_MemRegionsNumber].SizeInBytes = headAl.SizeInBytes;
		pm_MemRegions[pm_MemRegionsNumber].BaseAddress = baseAddress;
		pm_TotalMemInBytes += headAl.SizeInBytes;
		pm_MemRegionsNumber++;

		// recursion
		return InternalInit(nextBase, restAl...);
	}
	
private:
	voidptr                                 	m_BaseAddress;

public:
	MemoryRegionInfo							pm_MemRegions[MAX_MEM_REGIONS];
	u32											pm_MemRegionsNumber;
	u32											pm_TotalMemInBytes;
};

}

#endif // __HELICH_MEMORY_MANAGER_H__
