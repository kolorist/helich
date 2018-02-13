namespace helich {
    template <class TAllocRegion>
    void AllocRegionDebugInfoExtractor<TAllocRegion>::ExtractInfo(voidptr allocRegion, DebugMemBlock* memBlocks,
			const u32 maxSize, u32& numBlocks)
    {
		typedef TAllocRegion::AllocHeaderType HeaderType;

		HeaderType* lastAlloc = ((TAllocRegion*)allocRegion)->m_LastAlloc;
		HeaderType* currAlloc = lastAlloc;
		u32 numAllocBlocks = 0;
		while (currAlloc != nullptr) {
			FLORAL_ASSERT_MSG(numAllocBlocks <= maxSize, "Error when building Allocation Block list (not enough array size)");

			memBlocks[numAllocBlocks].pm_FrameSize = currAlloc->FrameSize;
			memBlocks[numAllocBlocks].pm_FrameAddress = (s8*)((u32)currAlloc - currAlloc->Adjustment);

			numAllocBlocks++;
			currAlloc = currAlloc->NextAlloc;
		}

		numBlocks = numAllocBlocks;
    }
}
