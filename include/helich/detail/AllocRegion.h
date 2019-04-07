#pragma once

// 3rd-party headers
#include <floral.h>

#include "helich/MemoryDebug.h"

namespace helich {
namespace detail {

	template <class t_alloc_header>
	class alloc_region {
    public:
        typedef t_alloc_header					alloc_header_t;

	public:
		alloc_region()
            : p_last_alloc(nullptr)
			, p_base_address(nullptr)
			, p_size_in_bytes(0)
			, p_used_bytes(0)
		{ }

	protected:
		~alloc_region()
		{ }

	public:
        alloc_header_t*							p_last_alloc;
		p8										p_base_address;
		size									p_size_in_bytes;
		size									p_used_bytes;

		// TODO: m_?
		floral::mutex							m_alloc_mutex;
	};

}

typedef void (*dbginfo_extractor_func_t)(voidptr, debug_memory_block*, const u32, u32&);

template <class t_alloc_region>
struct alloc_region_dbginfo_extractor {
	static void                             extract_info(voidptr i_allocRegion, debug_memory_block* i_memBlocks, const u32 i_maxSize, u32& o_numBlocks);
};

}

#include "AllocRegion.hpp"