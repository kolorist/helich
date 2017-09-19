#include "TrackingPolicies.h"
#include "StackWalker.h"

// 3rd-party headers
#include <cstring>

namespace helich {

	StackWalker									g_Walker;
	FixedAllocator<PoolScheme, sizeof(DebugEntry), NoTrackingPolicy> g_TrackingAllocator;

	//////////////////////////////////////////////////////////////////////////
	// Default Tracking Policy
	//////////////////////////////////////////////////////////////////////////

	u32 DefaultTrackingPolicy::m_NumAlloc = 0;
	
	void DefaultTrackingPolicy::Register(voidptr dataAddr, const u32 nBytes, const_cstr desc, const_cstr file, const u32 line)
	{
		// get memory header
		AllocHeaderType* memHeader = (AllocHeaderType*)dataAddr;

		// allocate new TrackingEntry
		DebugEntry* newEntry = g_TrackingAllocator.Allocate<DebugEntry>();

		// populate allocation information
		strcpy_s(newEntry->Description, 128, desc);
		newEntry->SizeInBytes = nBytes;
		newEntry->Address = dataAddr;
		newEntry->StackTrace[0] = 0;
		g_Walker.ShowCallstack(newEntry->StackTrace);

		// update memory header info
		memHeader->DebugInfo = newEntry;

		m_NumAlloc++;
	}

	void DefaultTrackingPolicy::Unregister(voidptr ptr)
	{
		// get memory header
		AllocHeaderType* memHeader = (AllocHeaderType*)ptr;

		// and free tracking info
		g_TrackingAllocator.Free(memHeader->DebugInfo);

		memHeader->DebugInfo = nullptr;

		m_NumAlloc--;
	}

}