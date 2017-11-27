#ifndef __HL_ALLOC_REGION_H__
#define __HL_ALLOC_REGION_H__

// 3rd-party headers
#include <stdaliases.h>

namespace helich {
namespace detail {

	template <class TAllocHeader>
	class AllocRegion {
	public:
		AllocRegion()
			: m_BaseAddress(nullptr)
			, m_SizeInBytes(0)
			, m_UsedBytes(0)
		{ }

	protected:
		~AllocRegion()
		{ }

	protected:
		s8*										m_BaseAddress;
		u32										m_SizeInBytes;
		u32										m_UsedBytes;
	};

}
}

#endif // __HL_ALLOC_REGION_H__
