#include "AllocSchemes.h"

// self-provided headers
#include "Utils.h"

// 3rd-party headers
#include <cassert>
#include <string.h>

namespace helich {
	//////////////////////////////////////////////////////////////////////////
	// Stack Allocation Scheme
	//////////////////////////////////////////////////////////////////////////
	template <class PTracking>
	StackScheme<PTracking>::StackScheme()
		: AllocRegion()
	{

	}

	template <class PTracking>
	StackScheme<PTracking>::~StackScheme()
	{
		m_BaseAddress = nullptr;
		m_CurrentMarker = nullptr;
		m_SizeInBytes = 0;
		m_LastAlloc = nullptr;
	}

	template <class PTracking>
	void StackScheme<PTracking>::MapTo(voidptr baseAddress, const u32 sizeInBytes, const_cstr name)
	{
		m_BaseAddress = (s8*)baseAddress;
		m_SizeInBytes = sizeInBytes;
		m_CurrentMarker = (s8*)baseAddress;
		m_LastAlloc = nullptr;
	}

	template <class PTracking>
	voidptr StackScheme<PTracking>::Allocate(const u32 nBytes)
	{
		// the whole stack frame size, count all headers, displacement, data, ...
		// ....[A..A][H..H][D..D][A'..A']
		// A: aligning-bytes    : always >= 1 byte
		// H: header
		// D: data
		// [A'..A']: not-free region but contains nothing   : >= 0 bytes
		// sizeof([A..A]) + sizeof([A'..A']) == HL_ALIGNMENT
		// stack header's ([H..H]) size always equals to HL_ALIGNMENT (min = 4 bytes)
		u32 frameSize = nBytes + HL_ALIGNMENT + sizeof(AllocHeaderType);
		// out of memory check
		assert((u32)m_CurrentMarker + frameSize <= (u32)m_BaseAddress + m_SizeInBytes);
		// start address of the frame
		s8* orgAddr = m_CurrentMarker;

		s8* headerAddr = (s8*)alignAddress(orgAddr);    // forward align, this is address of the header
		s8* dataAddr = headerAddr + sizeof(AllocHeaderType);       // sure-align address, this is the start of the data
		// reset data memory region
		memset(dataAddr, 0, nBytes);

		// save info about displacement and allocated frame size
		u32 displacement = (u32)headerAddr - (u32)orgAddr;
		AllocHeaderType* header = (AllocHeaderType*)headerAddr;
		header->NextAlloc = nullptr;
		header->PrevAlloc = m_LastAlloc;
		header->FrameSize = frameSize;
		header->Adjustment = displacement;
		if (m_LastAlloc != nullptr) {
			m_LastAlloc->NextAlloc = header;
		}

		m_LastAlloc = header;

		// increase marker
		m_CurrentMarker += frameSize;

		// register allocation
		PTracking::Register(header, nBytes, "no-desc", __FILE__, __LINE__);

		return reinterpret_cast<voidptr>(dataAddr);
	}

	template <class PTracking>
	void StackScheme<PTracking>::Free(voidptr pData)
	{
		// get the header position
		AllocHeaderType* header = (AllocHeaderType*)pData - 1;
		// now, we can get the frame size
		u32 frameSize = header->FrameSize;
		u32 displacement = header->Adjustment;

		// validate deallocation
		s8* orgAddr = (s8*)header - displacement;
		s8* lastAllocAddr = m_CurrentMarker - frameSize;

		assert(orgAddr == lastAllocAddr && "Invalid free: not in allocation order");

		// adjust header
		if (header->PrevAlloc)
			header->PrevAlloc->NextAlloc = nullptr;

		// unregister allocation
		PTracking::Unregister(header);

		m_LastAlloc = header->PrevAlloc;

		// reset memory region
		memset(orgAddr, 0, frameSize);
		// done validation, free memory
		m_CurrentMarker -= frameSize;
	}

	template <class PTracking>
	void StackScheme<PTracking>::FreeAll()
	{
		m_CurrentMarker = m_BaseAddress;
		memset(m_BaseAddress, 0, m_SizeInBytes);
	}

	//////////////////////////////////////////////////////////////////////////
	// Pooling Allocation Scheme
	//////////////////////////////////////////////////////////////////////////
	template <u32 UElemSize, class PTracking>
	PoolScheme<UElemSize, PTracking>::PoolScheme()
		: m_LastAlloc(nullptr)
		, m_NextFreeSlot(nullptr)
		, m_ElementSize(0)
		, m_ElementCount(0)
	{

	}

	template <u32 UElemSize, class PTracking>
	PoolScheme<UElemSize, PTracking>::~PoolScheme()
	{

	}

	template <u32 UElemSize, class PTracking>
	void PoolScheme<UElemSize, PTracking>::MapTo(voidptr baseAddress, const u32 sizeInBytes, const_cstr name)
	{
		m_ElementSize = ((UElemSize - 1) / HL_ALIGNMENT + 1) * HL_ALIGNMENT + sizeof(AllocHeaderType);
		m_ElementCount = sizeInBytes / m_ElementSize;
		m_BaseAddress = (s8*)baseAddress;
		m_SizeInBytes = sizeInBytes;
		// fill the assoc list
		for (u32 i = 0; i < m_ElementCount - 1; i++) {
			s8* addr = m_BaseAddress + i * m_ElementSize;
			s8* nextAddr = m_BaseAddress + (i + 1) * m_ElementSize;
			AllocHeaderType* header = (AllocHeaderType*)addr;
			header->NextAlloc = (AllocHeaderType*)nextAddr;
		}
		m_NextFreeSlot = (AllocHeaderType*)m_BaseAddress;
	}

	template <u32 UElemSize, class PTracking>
	voidptr PoolScheme<UElemSize, PTracking>::Allocate()
	{
		// TODO: out of memory assertion

		// we have return address right-away, the memory region was pre-aligned already
		//s8* headerAddr = (s8*)((u32)m_BaseAddress + (u32)m_NextFreeSlot);
		s8* headerAddr = (s8*)m_NextFreeSlot;
		s8* dataAddr = headerAddr + sizeof(AllocHeaderType);
		// next free slot is contained inside pooled element, update it by them
		// update header and next free slot
		AllocHeaderType* header = (AllocHeaderType*)headerAddr;
		m_NextFreeSlot = header->NextAlloc;
		header->NextAlloc = nullptr;
		header->PrevAlloc = m_LastAlloc;
		if (m_LastAlloc != nullptr) {
			m_LastAlloc->NextAlloc = header;
		}

		PTracking::Register(headerAddr, m_ElementSize, "no-desc", __FILE__, __LINE__);

		m_LastAlloc = header;
		// reset memory region
		memset(dataAddr, 0, UElemSize);

		return dataAddr;
	}

	template <u32 UElemSize, class PTracking>
	void PoolScheme<UElemSize, PTracking>::Free(voidptr pData)
	{
		// calculate position of the will-be-freed slot
		//u32 idx = ((u32)pData - (u32)m_BaseAddress) / m_ElementSize;
		AllocHeaderType* header = (AllocHeaderType*)((s8*)pData - sizeof(AllocHeaderType));

		// unregister tracking info
		PTracking::Unregister(header);

		if (header->NextAlloc) {
			header->NextAlloc->PrevAlloc = header->PrevAlloc;
		}
		if (header->PrevAlloc) {
			header->PrevAlloc->NextAlloc = header->NextAlloc;
		}
		// are we freeing the last allocation?
		if (header == m_LastAlloc) {
			// yes, then update the list's tail element
			m_LastAlloc = header->PrevAlloc;
		}

		memset(header, 0, m_ElementSize);

		// update this slot's next free slot to next free slot
		header->NextAlloc = m_NextFreeSlot;
		//*((u32*)headerAddr) = m_NextFreeIdx;

		// update next free slot to this slot's index
		//m_NextFreeIdx = idx;
		m_NextFreeSlot = header;
	}

	template <u32 UElemSize, class PTracking>
	void PoolScheme<UElemSize, PTracking>::FreeAll()
	{
		// TODO
	}


	//////////////////////////////////////////////////////////////////////////
	// Freelist Allocation Scheme
	//////////////////////////////////////////////////////////////////////////
	template <class PTracking>
	FreelistScheme<PTracking>::FreelistScheme()
		: AllocRegion()
		, m_FirstFreeBlock(nullptr)
		, k_MinFrameSize(sizeof(AllocHeaderType) + HL_ALIGNMENT + 1)
		, m_LastAlloc(nullptr)
		, pm_AllocCount(0)
		, pm_FreeCount(0)
	{

	}

	template <class PTracking>
	FreelistScheme<PTracking>::~FreelistScheme()
	{

	}

	template <class PTracking>
	void FreelistScheme<PTracking>::MapTo(voidptr baseAddress, const u32 sizeInBytes, const_cstr name)
	{
		m_BaseAddress = (s8*)baseAddress;
		m_SizeInBytes = sizeInBytes;

		m_FirstFreeBlock = (AllocHeaderType*)m_BaseAddress;
		m_FirstFreeBlock->FrameSize = m_SizeInBytes;
		m_FirstFreeBlock->Adjustment = 0;
		m_FirstFreeBlock->NextAlloc = nullptr;
		m_FirstFreeBlock->PrevAlloc = nullptr;
	}

	// inline services for allocation
	template <class PTracking>
	const bool FreelistScheme<PTracking>::CanFit(AllocHeaderType* header, const u32 nBytes)
	{
		return (header->FrameSize - HL_ALIGNMENT - sizeof(AllocHeaderType) >= nBytes);
	}

	template <class PTracking>
	const bool FreelistScheme<PTracking>::CanCreateNewBlock(AllocHeaderType* header, const u32 nBytes, const u32 minFrameSize)
	{
		u32 remaining = header->FrameSize - HL_ALIGNMENT - sizeof(AllocHeaderType) - nBytes;
		return (remaining >= minFrameSize);
	}

	template <class PTracking>
	voidptr FreelistScheme<PTracking>::Allocate(const u32 nBytes)
	{
		// first-fit strategy
		AllocHeaderType* currBlock = m_FirstFreeBlock;
		// search
		while (currBlock && !CanFit(currBlock, nBytes)) {
			currBlock = currBlock->NextAlloc;
		}

		if (currBlock) { // found it!
			voidptr dataAddr = (s8*)currBlock + sizeof(AllocHeaderType);
			// C1: a new free block needs to be created
			if (CanCreateNewBlock(currBlock, nBytes, k_MinFrameSize)) {
				u32 oldFrameSize = currBlock->FrameSize;
				// update frameSize of currBlock
				u32 currFrameSize = sizeof(AllocHeaderType) + nBytes + HL_ALIGNMENT;
				u8 disp = currBlock->Adjustment;
				currBlock->FrameSize = currFrameSize;

				// create new free block
				s8* unalignedNBStart = (s8*)currBlock - disp + currBlock->FrameSize;
				//s8* unalignedNBStart = (s8*)currBlock + currBlock->FrameSize;
				s8* nbStart = (s8*)alignAddress(unalignedNBStart);
				u32 nbDisp = (u32)nbStart - (u32)unalignedNBStart;
				u32 nbFrameSize = oldFrameSize - currFrameSize;
				AllocHeaderType* newBlock = (AllocHeaderType*)nbStart;
				// update pointers of new free block
				newBlock->NextAlloc = currBlock->NextAlloc;
				newBlock->PrevAlloc = currBlock->PrevAlloc;
				//newBlock->TrackingInfo = nullptr;
				newBlock->FrameSize = nbFrameSize;
				newBlock->Adjustment = nbDisp;

				// delete pointers on currBlock as it's already occupied
				currBlock->NextAlloc = nullptr;
				currBlock->PrevAlloc = nullptr;

				// update linked list
				if (newBlock->PrevAlloc) {
					newBlock->PrevAlloc->NextAlloc = newBlock;
				}
				if (newBlock->NextAlloc) {
					newBlock->NextAlloc->PrevAlloc = newBlock;
				}

				// update first free block
				if (currBlock == m_FirstFreeBlock) {
					m_FirstFreeBlock = newBlock;
				}
			}
			else {
				// C2: we can use all of this block
				if (currBlock->PrevAlloc) {
					currBlock->PrevAlloc->NextAlloc = currBlock->NextAlloc;
				}
				if (currBlock->NextAlloc) {
					currBlock->NextAlloc->PrevAlloc = currBlock->PrevAlloc;
				}
				// update first free block?
				if (m_FirstFreeBlock == currBlock) {
					m_FirstFreeBlock = currBlock->NextAlloc;
				}

				// delete pointers
				currBlock->NextAlloc = nullptr;
				currBlock->PrevAlloc = nullptr;
			}

			currBlock->NextAlloc = nullptr;
			currBlock->PrevAlloc = m_LastAlloc;
			if (m_LastAlloc != nullptr)
				m_LastAlloc->NextAlloc = currBlock;
			m_LastAlloc = currBlock;
			PTracking::Register(currBlock, nBytes, "no-desc", __FILE__, __LINE__);
			m_UsedBytes += currBlock->FrameSize;

			pm_AllocCount++;
			return dataAddr;
		}
		// nothing found, cannot allocate anything
		return nullptr;
	}

	// free a block, update the free list
	template <class PTracking>
	void FreelistScheme<PTracking>::FreeBlock(AllocHeaderType* block, AllocHeaderType* prevFree, AllocHeaderType* nextFree)
	{
		// erase its content
		s8* pData = (s8*)block + sizeof(AllocHeaderType);
		//memset(pData, 0, block->GetDataCapacity());
		memset(pData, 0, block->FrameSize - HL_ALIGNMENT - sizeof(AllocHeaderType));
		block->NextAlloc = nullptr;
		block->PrevAlloc = nullptr;
		//block->TrackingInfo = nullptr;

		// adjust pointers
		if (prevFree) {
			block->PrevAlloc = prevFree;
			prevFree->NextAlloc = block;
		}
		if (nextFree) {
			block->NextAlloc = nextFree;
			nextFree->PrevAlloc = block;
		}
	}

	// check if 2 blocks can be joined
	template <class PTracking>
	const bool FreelistScheme<PTracking>::CanJoin(AllocHeaderType* leftBlock, AllocHeaderType* rightBlock)
	{
		u32 leftEnd = (u32)leftBlock - leftBlock->Adjustment + leftBlock->FrameSize;
		u32 rightStart = (u32)rightBlock - rightBlock->Adjustment;
		return (leftEnd == rightStart);
	}

	// join 2 *free* blocks together
	template <class PTracking>
	const bool FreelistScheme<PTracking>::JoinBlocks(AllocHeaderType* leftBlock, AllocHeaderType* rightBlock)
	{
		if (!leftBlock || !rightBlock)
			return false;

		if (CanJoin(leftBlock, rightBlock)) {
			// adjust left block's pointers
			leftBlock->NextAlloc = rightBlock->NextAlloc;

			// update frameSize of left block
			leftBlock->FrameSize += rightBlock->FrameSize;

			if (rightBlock->NextAlloc)
				rightBlock->NextAlloc->PrevAlloc = leftBlock;

			// erase right block header as we don't need it anymore
			memset(rightBlock, 0, sizeof(AllocHeaderType));
			return true;
		}
		else return false;
	}

	template <class PTracking>
	void FreelistScheme<PTracking>::Free(voidptr pData)
	{
		AllocHeaderType* releaseBlock = (AllocHeaderType*)((s8*)pData - sizeof(AllocHeaderType));
		m_UsedBytes -= releaseBlock->FrameSize;
		PTracking::Unregister(releaseBlock);
		pm_FreeCount++;

		if (releaseBlock->NextAlloc)
			releaseBlock->NextAlloc->PrevAlloc = releaseBlock->PrevAlloc;
		if (releaseBlock->PrevAlloc)
			releaseBlock->PrevAlloc->NextAlloc = releaseBlock->NextAlloc;
		if (releaseBlock == m_LastAlloc) {
			m_LastAlloc = releaseBlock->PrevAlloc;
		}

		// search for nearest-after free block
		AllocHeaderType* nextFree = m_FirstFreeBlock;
		while (nextFree &&
			((u32)nextFree <= (u32)releaseBlock)) {
			nextFree = nextFree->NextAlloc;
		}
		AllocHeaderType* prevFree = nextFree->PrevAlloc;

		// free releaseBlock
		FreeBlock(releaseBlock, prevFree, nextFree);

		// update first free block
		if ((u32)releaseBlock < (u32)m_FirstFreeBlock) {
			m_FirstFreeBlock = releaseBlock;
		}
		// join blocks if possible
		if (JoinBlocks(prevFree, releaseBlock))
			JoinBlocks(prevFree, nextFree);
		else JoinBlocks(releaseBlock, nextFree);
	}

	template <class PTracking>
	void FreelistScheme<PTracking>::FreeAll()
	{
		memset(m_BaseAddress, 0, m_SizeInBytes);

		m_FirstFreeBlock = (AllocHeaderType*)m_BaseAddress;
		m_FirstFreeBlock->FrameSize = m_SizeInBytes;
		m_FirstFreeBlock->Adjustment = 0;
		m_FirstFreeBlock->NextAlloc = nullptr;
		m_FirstFreeBlock->PrevAlloc = nullptr;
	}
}
