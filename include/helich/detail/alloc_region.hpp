#include <string.h>

namespace helich
{

template <class t_alloc_region>
void alloc_region_dbginfo_extractor<t_alloc_region>::extract_info(voidptr i_allocRegion, debug_memory_block* i_memBlocks,
		const u32 i_maxSize, u32& o_numBlocks)
{
	typedef typename t_alloc_region::alloc_header_t	header_t;

	t_alloc_region* allocRegion = (t_alloc_region*)i_allocRegion;

	floral::lock_guard memGuard(allocRegion->m_alloc_mutex);

	header_t* lastAlloc = allocRegion->p_last_alloc;
	header_t* currAlloc = lastAlloc;
	u32 numAllocBlocks = 0;
	while (currAlloc != nullptr)
	{
		FLORAL_ASSERT_MSG(numAllocBlocks <= i_maxSize, "Error when building Allocation Block list (not enough array size)");

		i_memBlocks[numAllocBlocks].frame_size = currAlloc->frame_size;
		strcpy(i_memBlocks[numAllocBlocks].description, currAlloc->description);
		i_memBlocks[numAllocBlocks].frame_address = (p8)((aptr)currAlloc - currAlloc->adjustment);

		numAllocBlocks++;
		currAlloc = currAlloc->prev_alloc;
	}

	o_numBlocks = numAllocBlocks;
}

}
