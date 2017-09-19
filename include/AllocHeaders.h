#ifndef __HL_ALLOC_HEADERS_H__
#define __HL_ALLOC_HEADERS_H__

// 3rd-party headers
#include <stdaliases.h>

namespace helich {

	/*
	 * types of allocation header:
	 *	> VariableSize
	 *		>> Tracked
	 *		>> Untracked
	 *	> FixedSize
	 *		>> Tracked
	 *		>> Untracked
	 */

	template <class TTrackingHeader>
	struct FixedSizeAllocHeader : TTrackingHeader {
		FixedSizeAllocHeader*					NextAlloc;
		FixedSizeAllocHeader*					PrevAlloc;
	};

	template <class TTrackingHeader>
	struct VariableSizeAllocHeader : TTrackingHeader {
		VariableSizeAllocHeader*				NextAlloc;
		VariableSizeAllocHeader*				PrevAlloc;
		u32										FrameSize;
		u32										Adjustment;
	};

	struct DebugEntry;
	struct TrackedAllocHeader {
		DebugEntry*								DebugInfo;
	};

	struct UntrackedAllocHeader {
	};
}

#endif // __HL_ALLOC_HEADERS_H__