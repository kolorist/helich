#ifndef __HL_TRACKING_POLICIES_H__
#define __HL_TRACKING_POLICIES_H__

// self-provided headers
#include "AllocHeaders.h"
#include "Allocator.h"
#include "AllocSchemes.h"

// 3rd-party headers
#include <stdaliases.h>

namespace helich {

	struct DebugEntry {
		voidptr									Address;
		u32										SizeInBytes;
		c8										Description[128];
		c8										StackTrace[2048];

		DebugEntry()
		{}
	};

	class DefaultTrackingPolicy {
	public:
		typedef TrackedAllocHeader				AllocHeaderType;

	public:
		static void								Register(voidptr dataAddr, const u32 nBytes, const_cstr desc, const_cstr file, const u32 line);
		static void								Unregister(voidptr ptr);

	private:
		static u32								m_NumAlloc;
	};

	// This policy will be used mostly by 'release' build

	class NoTrackingPolicy {
	public:
		typedef UntrackedAllocHeader			AllocHeaderType;

	public:
		static void								Register(voidptr dataAddr, const u32 nBytes, const_cstr desc, const_cstr file, const u32 line)	{}
		static void								Unregister(voidptr ptr)																			{}
	};
}

#endif // __HL_TRACKING_POLICIES_H__