#ifndef __HL_ALLOCATION_TRACKER_H__
#define __HL_ALLOCATION_TRACKER_H__

#include "macros.h"

#include "PoolAllocator.h"

#include <stdaliases.h>

BEGIN_HELICH_NAMESPACE

class AllocationTracker {
public:
	static const u32				k_HeaderPoolSize = 512;

public:
	AllocationTracker();
	~AllocationTracker();

	void										Initialize();
	void										RegisterAllocation(voidptr ptr, const u32 nbytes, const_cstr desc, const_cstr file, const u32 line);
	void										UnregisterAllocation(voidptr ptr);
	void										DoSanityCheck();
	const u32									GetAllocationCount() const		{ return m_NumAlloc; }
	const u32									GetFreeCount() const			{ return m_NumFree; }

private:
	u32											m_NumAlloc;
	u32											m_NumFree;
};

extern AllocationTracker						g_HLAllocationTracker;

END_HELICH_NAMESPACE

#endif // __HL_ALLOCATION_TRACKER_H__
