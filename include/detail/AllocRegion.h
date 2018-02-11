#ifndef __HL_ALLOC_REGION_H__
#define __HL_ALLOC_REGION_H__

// 3rd-party headers
#include <stdaliases.h>

#include "MemoryDebug.h"

namespace helich {
namespace detail {

	template <class TAllocHeader>
	class AllocRegion {
    public:
        typedef TAllocHeader                    AllocHeaderType;

	public:
		AllocRegion()
			: m_BaseAddress(nullptr)
			, m_SizeInBytes(0)
			, m_UsedBytes(0)
            , m_LastAlloc(nullptr)
		{ }

	protected:
		~AllocRegion()
		{ }

	protected:
        TAllocHeader*                           m_LastAlloc;
		s8*										m_BaseAddress;
		u32										m_SizeInBytes;
		u32										m_UsedBytes;
	};

}

    typedef void (*AllocRegionDebugInfoExtractFunc)(DebugMemBlock*, const u32, u32&);

    template <class TAllocRegion>
    struct AllocRegionDebugInfoExtractor {
        static void                             ExtractInfo(DebugMemBlock* memBlocks, const u32 maxSize, u32& numBlocks);
    };
}

#include "AllocRegion.hpp"

#endif // __HL_ALLOC_REGION_H__
