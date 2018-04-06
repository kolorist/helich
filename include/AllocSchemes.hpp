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
	template <class t_tracking>
	stack_scheme<t_tracking>::stack_scheme()
		: alloc_region()
	{

	}

	template <class t_tracking>
	stack_scheme<t_tracking>::~stack_scheme()
	{
		p_base_address = nullptr;
		m_current_marker = nullptr;
		p_size_in_bytes = 0;
	}

	template <class t_tracking>
	void stack_scheme<t_tracking>::MapTo(voidptr i_baseAddress, const size i_sizeInBytes, const_cstr i_name)
	{
		floral::lock_guard memGuard(m_alloc_mutex);
		p_base_address = (s8*)i_baseAddress;
		p_size_in_bytes = i_sizeInBytes;
		m_current_marker = (s8*)i_baseAddress;
	}

	template <class t_tracking>
	voidptr stack_scheme<t_tracking>::Allocate(const size i_bytes)
	{
		floral::lock_guard memGuard(m_alloc_mutex);
		// the whole stack frame size, count all headers, displacement, data, ...
		// ....[A..A][H..H][D..D][A'..A']
		// A: aligning-bytes    : always >= 1 byte
		// H: header
		// D: data
		// [A'..A']: not-free region but contains nothing   : >= 0 bytes
		// sizeof([A..A]) + sizeof([A'..A']) == HL_ALIGNMENT
		// stack header's ([H..H]) size always equals to HL_ALIGNMENT (min = 4 bytes)
		u32 frameSize = i_bytes + HL_ALIGNMENT + sizeof(alloc_header_t);
		// out of memory check
		assert((u32)m_current_marker + frameSize <= (u32)p_base_address + p_size_in_bytes);
		// start address of the frame
		s8* orgAddr = m_current_marker;

		s8* headerAddr = (s8*)alignAddress(orgAddr);    // forward align, this is address of the header
		s8* dataAddr = headerAddr + sizeof(alloc_header_t);       // sure-align address, this is the start of the data
		// reset data memory region
		memset(dataAddr, 0, i_bytes);

		// save info about displacement and allocated frame size
		u32 displacement = (u32)headerAddr - (u32)orgAddr;
		alloc_header_t* header = (alloc_header_t*)headerAddr;
		header->next_alloc = nullptr;
		header->prev_alloc = p_last_alloc;
		header->frame_size = frameSize;
		header->adjustment = displacement;
		if (p_last_alloc != nullptr) {
			p_last_alloc->next_alloc = header;
		}

		p_last_alloc = header;

		// increase marker
		m_current_marker += frameSize;

		// register allocation
		t_tracking::register(header, i_bytes, "no-desc", __FILE__, __LINE__);

		return static_cast<voidptr>(dataAddr);
	}

	template <class t_tracking>
	void stack_scheme<t_tracking>::free(voidptr i_data)
	{
		floral::lock_guard memGuard(m_alloc_mutex);
		// get the header position
		alloc_header_t* header = (alloc_header_t*)i_data - 1;
		// now, we can get the frame size
		u32 frameSize = header->frame_size;
		u32 displacement = header->adjustment;

		// validate deallocation
		s8* orgAddr = (s8*)header - displacement;
		s8* lastAllocAddr = m_current_marker - frameSize;

		assert(orgAddr == lastAllocAddr && "Invalid free: not in allocation order");

		// adjust header
		if (header->prev_alloc)
			header->prev_alloc->next_alloc = nullptr;

		// unregister allocation
		t_tracking::unregister(header);

		p_last_alloc = header->prev_alloc;

		// reset memory region
		memset(orgAddr, 0, frameSize);
		// done validation, free memory
		m_current_marker -= frameSize;
	}

	template <class t_tracking>
	void stack_scheme<t_tracking>::free_all()
	{
		floral::lock_guard memGuard(m_alloc_mutex);
		m_current_marker = p_base_address;
		memset(p_base_address, 0, p_size_in_bytes);
	}

	//////////////////////////////////////////////////////////////////////////
	// Pooling Allocation Scheme
	//////////////////////////////////////////////////////////////////////////
	template <u32 t_elem_size, class t_tracking>
	pool_scheme<t_elem_size, t_tracking>::pool_scheme()
		: alloc_region()//m_LastAlloc(nullptr)
		, m_next_free_slot(nullptr)
		, m_element_size(0)
		, m_element_count(0)
	{

	}

	template <u32 t_elem_size, class t_tracking>
	pool_scheme<t_elem_size, t_tracking>::~pool_scheme()
	{

	}

	template <u32 t_elem_size, class t_tracking>
	void pool_scheme<t_elem_size, t_tracking>::MapTo(voidptr baseAddress, const u32 sizeInBytes, const_cstr name)
	{
		floral::lock_guard memGuard(m_AllocMutex);
		m_element_size = ((t_elem_size - 1) / HL_ALIGNMENT + 1) * HL_ALIGNMENT + sizeof(AllocHeaderType);
		m_element_count = sizeInBytes / m_element_size;
		m_BaseAddress = (s8*)baseAddress;
		m_SizeInBytes = sizeInBytes;
		// fill the assoc list
		for (u32 i = 0; i < m_element_count - 1; i++) {
			s8* addr = m_BaseAddress + i * m_element_size;
			s8* nextAddr = m_BaseAddress + (i + 1) * m_element_size;
			AllocHeaderType* header = (AllocHeaderType*)addr;
			header->NextAlloc = (AllocHeaderType*)nextAddr;
			header->FrameSize = m_element_size;
			header->Adjustment = 0;
		}
		m_next_free_slot = (AllocHeaderType*)m_BaseAddress;
	}

	template <u32 t_elem_size, class t_tracking>
	voidptr pool_scheme<t_elem_size, t_tracking>::Allocate()
	{
		floral::lock_guard memGuard(m_AllocMutex);
		// TODO: out of memory assertion

		// we have return address right-away, the memory region was pre-aligned already
		//s8* headerAddr = (s8*)((u32)m_BaseAddress + (u32)m_next_free_slot);
		s8* headerAddr = (s8*)m_next_free_slot;
		s8* dataAddr = headerAddr + sizeof(AllocHeaderType);
		// next free slot is contained inside pooled element, update it by them
		// update header and next free slot
		AllocHeaderType* header = (AllocHeaderType*)headerAddr;
		m_next_free_slot = header->NextAlloc;
		header->NextAlloc = nullptr;
		header->PrevAlloc = m_LastAlloc;
		if (m_LastAlloc != nullptr) {
			m_LastAlloc->NextAlloc = header;
		}

		t_tracking::Register(headerAddr, m_element_size, "no-desc", __FILE__, __LINE__);

		m_LastAlloc = header;
		// reset memory region
		memset(dataAddr, 0, t_elem_size);

		return dataAddr;
	}

	template <u32 t_elem_size, class t_tracking>
	void pool_scheme<t_elem_size, t_tracking>::Free(voidptr pData)
	{
		floral::lock_guard memGuard(m_AllocMutex);
		// calculate position of the will-be-freed slot
		AllocHeaderType* header = (AllocHeaderType*)((s8*)pData - sizeof(AllocHeaderType));

		// unregister tracking info
		t_tracking::Unregister(header);

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

		memset(header, 0, m_element_size);

		// update this slot's next free slot to next free slot
		header->NextAlloc = m_next_free_slot;
		header->FrameSize = m_element_size;
		header->Adjustment = 0;
		//*((u32*)headerAddr) = m_NextFreeIdx;

		// update next free slot to this slot's index
		//m_NextFreeIdx = idx;
		m_next_free_slot = header;
	}

	template <u32 t_elem_size, class t_tracking>
	void pool_scheme<t_elem_size, t_tracking>::FreeAll()
	{
		floral::lock_guard memGuard(m_AllocMutex);
		// TODO
	}


	//////////////////////////////////////////////////////////////////////////
	// Freelist Allocation Scheme
	//////////////////////////////////////////////////////////////////////////
	template <class t_tracking>
	FreelistScheme<t_tracking>::FreelistScheme()
		: alloc_region()
		, m_FirstFreeBlock(nullptr)
		, k_MinFrameSize(sizeof(AllocHeaderType) + HL_ALIGNMENT + 1)
		//, m_LastAlloc(nullptr)
		, pm_AllocCount(0)
		, pm_FreeCount(0)
	{

	}

	template <class t_tracking>
	FreelistScheme<t_tracking>::~FreelistScheme()
	{

	}

	template <class t_tracking>
	void FreelistScheme<t_tracking>::MapTo(voidptr baseAddress, const u32 sizeInBytes, const_cstr name)
	{
		floral::lock_guard memGuard(m_AllocMutex);
		m_BaseAddress = (s8*)baseAddress;
		m_SizeInBytes = sizeInBytes;

		m_FirstFreeBlock = (AllocHeaderType*)m_BaseAddress;
		m_FirstFreeBlock->FrameSize = m_SizeInBytes;
		m_FirstFreeBlock->Adjustment = 0;
		m_FirstFreeBlock->NextAlloc = nullptr;
		m_FirstFreeBlock->PrevAlloc = nullptr;
	}

	// inline services for allocation
	template <class t_tracking>
	const bool FreelistScheme<t_tracking>::CanFit(AllocHeaderType* header, const u32 nBytes)
	{
		return (header->FrameSize - HL_ALIGNMENT - sizeof(AllocHeaderType) >= nBytes);
	}

	template <class t_tracking>
	const bool FreelistScheme<t_tracking>::CanCreateNewBlock(AllocHeaderType* header, const u32 nBytes, const u32 minFrameSize)
	{
		u32 remaining = header->FrameSize - HL_ALIGNMENT - sizeof(AllocHeaderType) - nBytes;
		return (remaining >= minFrameSize);
	}

	template <class t_tracking>
	voidptr FreelistScheme<t_tracking>::Allocate(const u32 nBytes)
	{
		floral::lock_guard memGuard(m_AllocMutex);
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
			t_tracking::Register(currBlock, nBytes, "no-desc", __FILE__, __LINE__);
			m_UsedBytes += currBlock->FrameSize;

			pm_AllocCount++;
			return dataAddr;
		}
		// nothing found, cannot allocate anything
		return nullptr;
	}

	// free a block, update the free list
	template <class t_tracking>
	void FreelistScheme<t_tracking>::FreeBlock(AllocHeaderType* block, AllocHeaderType* prevFree, AllocHeaderType* nextFree)
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
	template <class t_tracking>
	const bool FreelistScheme<t_tracking>::CanJoin(AllocHeaderType* leftBlock, AllocHeaderType* rightBlock)
	{
		u32 leftEnd = (u32)leftBlock - leftBlock->Adjustment + leftBlock->FrameSize;
		u32 rightStart = (u32)rightBlock - rightBlock->Adjustment;
		return (leftEnd == rightStart);
	}

	// join 2 *free* blocks together
	template <class t_tracking>
	const bool FreelistScheme<t_tracking>::JoinBlocks(AllocHeaderType* leftBlock, AllocHeaderType* rightBlock)
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

	template <class t_tracking>
	void FreelistScheme<t_tracking>::Free(voidptr pData)
	{
		floral::lock_guard memGuard(m_AllocMutex);
		AllocHeaderType* releaseBlock = (AllocHeaderType*)((s8*)pData - sizeof(AllocHeaderType));
		m_UsedBytes -= releaseBlock->FrameSize;
		t_tracking::Unregister(releaseBlock);
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

	template <class t_tracking>
	void FreelistScheme<t_tracking>::FreeAll()
	{
		floral::lock_guard memGuard(m_AllocMutex);
		memset(m_BaseAddress, 0, m_SizeInBytes);

		m_FirstFreeBlock = (AllocHeaderType*)m_BaseAddress;
		m_FirstFreeBlock->FrameSize = m_SizeInBytes;
		m_FirstFreeBlock->Adjustment = 0;
		m_FirstFreeBlock->NextAlloc = nullptr;
		m_FirstFreeBlock->PrevAlloc = nullptr;
	}
}
