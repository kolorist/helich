namespace helich {
    template <class t_alloc_region>
    void alloc_region_dbginfo_extractor<t_alloc_region>::extract_info(voidptr i_allocRegion, debug_memory_block* i_memBlocks,
			const u32 i_maxSize, u32& o_numBlocks)
    {
		typedef typename t_alloc_region::alloc_header_t	header_t;

		header_t* lastAlloc = ((t_alloc_region*)i_allocRegion)->p_last_alloc;
		header_t* currAlloc = lastAlloc;
		u32 numAllocBlocks = 0;
		while (currAlloc != nullptr) {
			FLORAL_ASSERT_MSG(numAllocBlocks <= i_maxSize, "Error when building Allocation Block list (not enough array size)");

			i_memBlocks[numAllocBlocks].frame_size = currAlloc->frame_size;
			i_memBlocks[numAllocBlocks].frame_address = (p8)((aptr)currAlloc - currAlloc->adjustment);

			numAllocBlocks++;
			currAlloc = currAlloc->next_alloc;
		}

		o_numBlocks = numAllocBlocks;
    }
}
