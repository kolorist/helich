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
			, m_LastAlloc(nullptr)
		{ }

	protected:
		~AllocRegion()
		{ }

	protected:
		s8*										m_BaseAddress;
		u32										m_SizeInBytes;

		TAllocHeader*							m_LastAlloc;
	};

}
}

#endif // __HL_ALLOC_REGION_H__