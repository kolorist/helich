#include "AllocSchemes.h"

// self-provided headers
#include "Utils.h"

// 3rd-party headers
#include <cassert>
#include <string.h>

// WARNING: enable this will make sure that the memory region which is allocated or freed will be zero out before
// giving to user or putting back into the free regions, but it will affect heavily the performance.
//#define ZERO_OUT_MEMORY

namespace helich {
//////////////////////////////////////////////////////////////////////////
// Stack Allocation Scheme
//////////////////////////////////////////////////////////////////////////
template <class t_tracking>
stack_scheme<t_tracking>::stack_scheme()
	: alloc_region_t()
{

}

template <class t_tracking>
stack_scheme<t_tracking>::~stack_scheme()
{
	alloc_region_t::p_base_address = nullptr;
	m_current_marker = nullptr;
	alloc_region_t::p_size_in_bytes = 0;
}

template <class t_tracking>
void stack_scheme<t_tracking>::map_to(voidptr i_baseAddress, const size i_sizeInBytes, const_cstr i_name)
{
	floral::lock_guard memGuard(m_alloc_mutex);
	alloc_region_t::p_base_address = (p8)i_baseAddress;
	alloc_region_t::p_size_in_bytes = i_sizeInBytes;
	m_current_marker = (p8)i_baseAddress;
}

template <class t_tracking>
voidptr stack_scheme<t_tracking>::allocate(const size i_bytes)
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
	size frame_size = i_bytes + HL_ALIGNMENT + sizeof(alloc_header_t);
	// out of memory check
	assert((aptr)m_current_marker + frame_size <= (aptr)alloc_region_t::p_base_address + alloc_region_t::p_size_in_bytes);
	// start address of the frame
	p8 orgAddr = m_current_marker;

	p8 headerAddr = (p8)align_address(orgAddr);    // forward align, this is address of the header
	p8 dataAddr = headerAddr + sizeof(alloc_header_t);       // sure-align address, this is the start of the data
	// reset data memory region
#if defined(ZERO_OUT_MEMORY)
	memset(dataAddr, 0, i_bytes);
#endif

	// save info about displacement and allocated frame size
	size displacement = (aptr)headerAddr - (aptr)orgAddr;
	alloc_header_t* header = (alloc_header_t*)headerAddr;
	header->next_alloc = nullptr;
	header->prev_alloc = alloc_region_t::p_last_alloc;
	header->frame_size = frame_size;
	header->adjustment = displacement;
	if (alloc_region_t::p_last_alloc != nullptr) {
		alloc_region_t::p_last_alloc->next_alloc = header;
	}

	alloc_region_t::p_last_alloc = header;

	// increase marker
	m_current_marker += frame_size;

	// register allocation
	t_tracking::register_allocation(header, i_bytes, "no-desc", __FILE__, __LINE__);

	return static_cast<voidptr>(dataAddr);
}

template <class t_tracking>
voidptr stack_scheme<t_tracking>::reallocate(voidptr i_data, const size i_newBytes)
{
	floral::lock_guard memGuard(m_alloc_mutex);
	voidptr newAllocation = allocate(i_newBytes);

	if (newAllocation != nullptr) {
		alloc_header_t* oldHeader = (alloc_header_t*)i_data - 1;
		size dataSizeBytes = oldHeader->frame_size - sizeof(alloc_header_t) - HL_ALIGNMENT;

		// memcpy
		memcpy(newAllocation, i_data, dataSizeBytes);

		return newAllocation;
	}
	return nullptr;
}

template <class t_tracking>
void stack_scheme<t_tracking>::free(voidptr i_data)
{
	floral::lock_guard memGuard(m_alloc_mutex);
	// get the header position
	alloc_header_t* header = (alloc_header_t*)i_data - 1;
	// now, we can get the frame size
	size frame_size = header->frame_size;
	size displacement = header->adjustment;

	// validate deallocation
	p8 orgAddr = (p8)header - displacement;
	p8 lastAllocAddr = m_current_marker - frame_size;

	assert(orgAddr == lastAllocAddr && "Invalid free: not in allocation order");

	// adjust header
	if (header->prev_alloc)
		header->prev_alloc->next_alloc = nullptr;

	// unregister allocation
	t_tracking::unregister_allocation(header);

	alloc_region_t::p_last_alloc = header->prev_alloc;

	// reset memory region
#if defined(ZERO_OUT_MEMORY)
	memset(orgAddr, 0, frame_size);
#endif
	// done validation, free memory
	m_current_marker -= frame_size;
}

template <class t_tracking>
void stack_scheme<t_tracking>::free_all()
{
	floral::lock_guard memGuard(m_alloc_mutex);
	m_current_marker = alloc_region_t::p_base_address;
	p_last_alloc = nullptr;
	p_used_bytes = 0;
#if defined(ZERO_OUT_MEMORY)
	memset(alloc_region_t::p_base_address, 0, alloc_region_t::p_size_in_bytes);
#endif
}

//////////////////////////////////////////////////////////////////////////
// Pooling Allocation Scheme
//////////////////////////////////////////////////////////////////////////
template <size t_elem_size, class t_tracking>
pool_scheme<t_elem_size, t_tracking>::pool_scheme()
	: alloc_region_t()//alloc_region_t::p_last_alloc(nullptr)
	, m_next_free_slot(nullptr)
	, m_element_size(0)
	, m_element_count(0)
{

}

template <size t_elem_size, class t_tracking>
pool_scheme<t_elem_size, t_tracking>::~pool_scheme()
{

}

template <size t_elem_size, class t_tracking>
void pool_scheme<t_elem_size, t_tracking>::map_to(voidptr i_baseAddress, const size i_sizeInBytes, const_cstr i_name)
{
	floral::lock_guard memGuard(m_alloc_mutex);
	m_element_size = ((t_elem_size - 1) / HL_ALIGNMENT + 1) * HL_ALIGNMENT + sizeof(alloc_header_t);
	m_element_count = (u32)(i_sizeInBytes / m_element_size);
	alloc_region_t::p_base_address = (p8)i_baseAddress;
	alloc_region_t::p_size_in_bytes = i_sizeInBytes;
	// fill the assoc list
	for (u32 i = 0; i < m_element_count - 1; i++) {
		p8 addr = alloc_region_t::p_base_address + i * m_element_size;
		p8 nextAddr = alloc_region_t::p_base_address + (i + 1) * m_element_size;
		alloc_header_t* header = (alloc_header_t*)addr;
		header->next_alloc = (alloc_header_t*)nextAddr;
		header->frame_size = m_element_size;
		header->adjustment = 0;
	}
	m_next_free_slot = (alloc_header_t*)alloc_region_t::p_base_address;
}

template <size t_elem_size, class t_tracking>
voidptr pool_scheme<t_elem_size, t_tracking>::allocate()
{
	floral::lock_guard memGuard(m_alloc_mutex);
	// TODO: out of memory assertion

	// we have return address right-away, the memory region was pre-aligned already
	//p8 headerAddr = (p8)((aptr)alloc_region_t::p_base_address + (aptr)m_next_free_slot);
	p8 headerAddr = (p8)m_next_free_slot;
	p8 dataAddr = headerAddr + sizeof(alloc_header_t);
	// next free slot is contained inside pooled element, update it by them
	// update header and next free slot
	alloc_header_t* header = (alloc_header_t*)headerAddr;
	m_next_free_slot = header->next_alloc;
	header->next_alloc = nullptr;
	header->prev_alloc = alloc_region_t::p_last_alloc;
	if (alloc_region_t::p_last_alloc != nullptr) {
		alloc_region_t::p_last_alloc->next_alloc = header;
	}

	t_tracking::register_allocation(headerAddr, m_element_size, "no-desc", __FILE__, __LINE__);

	alloc_region_t::p_last_alloc = header;
	// reset memory region
#if defined(ZERO_OUT_MEMORY)
	memset(dataAddr, 0, t_elem_size);
#endif

	return dataAddr;
}

template <size t_elem_size, class t_tracking>
void pool_scheme<t_elem_size, t_tracking>::free(voidptr i_data)
{
	floral::lock_guard memGuard(m_alloc_mutex);
	// calculate position of the will-be-freed slot
	alloc_header_t* header = (alloc_header_t*)((p8)i_data - sizeof(alloc_header_t));

	// unregister tracking info
	t_tracking::unregister_allocation(header);

	if (header->next_alloc) {
		header->next_alloc->prev_alloc = header->prev_alloc;
	}
	if (header->prev_alloc) {
		header->prev_alloc->next_alloc = header->next_alloc;
	}
	// are we freeing the last allocation?
	if (header == alloc_region_t::p_last_alloc) {
		// yes, then update the list's tail element
		alloc_region_t::p_last_alloc = header->prev_alloc;
	}

#if defined(ZERO_OUT_MEMORY)
	memset(header, 0, m_element_size);
#endif

	// update this slot's next free slot to next free slot
	header->next_alloc = m_next_free_slot;
	header->frame_size = m_element_size;
	header->adjustment = 0;
	//*((u32*)headerAddr) = m_NextFreeIdx;

	// update next free slot to this slot's index
	//m_NextFreeIdx = idx;
	m_next_free_slot = header;
}

template <size t_elem_size, class t_tracking>
void pool_scheme<t_elem_size, t_tracking>::free_all()
{
	floral::lock_guard memGuard(m_alloc_mutex);
	// TODO
}


//////////////////////////////////////////////////////////////////////////
// Freelist Allocation Scheme
//////////////////////////////////////////////////////////////////////////
template <class t_tracking>
freelist_scheme<t_tracking>::freelist_scheme()
	: alloc_region_t()
	, m_first_free_block(nullptr)
	, k_min_frame_size(sizeof(alloc_header_t) + HL_ALIGNMENT + 1)
	//, alloc_region_t::p_last_alloc(nullptr)
	, p_alloc_count(0)
	, p_free_count(0)
{

}

template <class t_tracking>
freelist_scheme<t_tracking>::~freelist_scheme()
{

}

template <class t_tracking>
void freelist_scheme<t_tracking>::map_to(voidptr i_baseAddress, const size i_sizeInBytes, const_cstr i_name)
{
	floral::lock_guard memGuard(m_alloc_mutex);
	alloc_region_t::p_base_address = (p8)i_baseAddress;
	alloc_region_t::p_size_in_bytes = i_sizeInBytes;

	m_first_free_block = (alloc_header_t*)alloc_region_t::p_base_address;
	m_first_free_block->frame_size = alloc_region_t::p_size_in_bytes;
	m_first_free_block->adjustment = 0;
	m_first_free_block->next_alloc = nullptr;
	m_first_free_block->prev_alloc = nullptr;
}

// inline services for allocation
template <class t_tracking>
const bool freelist_scheme<t_tracking>::can_fit(alloc_header_t* i_header, const size i_bytes)
{
	return (i_header->frame_size - HL_ALIGNMENT - sizeof(alloc_header_t) >= i_bytes);
}

template <class t_tracking>
const bool freelist_scheme<t_tracking>::can_create_new_block(alloc_header_t* i_header, const size i_bytes, const size i_minFrameSize)
{
	size remaining = i_header->frame_size - HL_ALIGNMENT - sizeof(alloc_header_t) - i_bytes;
	return (remaining >= i_minFrameSize);
}

template <class t_tracking>
voidptr freelist_scheme<t_tracking>::allocate(const size i_bytes)
{
	floral::lock_guard memGuard(m_alloc_mutex);
	// first-fit strategy
	alloc_header_t* currBlock = m_first_free_block;
	// search
	while (currBlock && !can_fit(currBlock, i_bytes)) {
		currBlock = currBlock->next_alloc;
	}

	if (currBlock) { // found it!
		voidptr dataAddr = (p8)currBlock + sizeof(alloc_header_t);
		// C1: a new free block needs to be created
		if (can_create_new_block(currBlock, i_bytes, k_min_frame_size)) {
			size oldFrameSize = currBlock->frame_size;
			// update frame_size of currBlock
			size currFrameSize = sizeof(alloc_header_t) + i_bytes + HL_ALIGNMENT;
			size disp = currBlock->adjustment;
			currBlock->frame_size = currFrameSize;

			// create new free block
			p8 unalignedNBStart = (p8)currBlock - disp + currBlock->frame_size;
			//p8 unalignedNBStart = (p8)currBlock + currBlock->frame_size;
			p8 nbStart = (p8)align_address(unalignedNBStart);
			aptr nbDisp = (aptr)nbStart - (aptr)unalignedNBStart;
			size nbFrameSize = oldFrameSize - currFrameSize;
			alloc_header_t* newBlock = (alloc_header_t*)nbStart;
			// update pointers of new free block
			newBlock->next_alloc = currBlock->next_alloc;
			newBlock->prev_alloc = currBlock->prev_alloc;
			//newBlock->TrackingInfo = nullptr;
			newBlock->frame_size = nbFrameSize;
			newBlock->adjustment = nbDisp;

			// delete pointers on currBlock as it's already occupied
			currBlock->next_alloc = nullptr;
			currBlock->prev_alloc = nullptr;

			// update linked list
			if (newBlock->prev_alloc) {
				newBlock->prev_alloc->next_alloc = newBlock;
			}
			if (newBlock->next_alloc) {
				newBlock->next_alloc->prev_alloc = newBlock;
			}

			// update first free block
			if (currBlock == m_first_free_block) {
				m_first_free_block = newBlock;
			}
		}
		else {
			// C2: we can use all of this block
			if (currBlock->prev_alloc) {
				currBlock->prev_alloc->next_alloc = currBlock->next_alloc;
			}
			if (currBlock->next_alloc) {
				currBlock->next_alloc->prev_alloc = currBlock->prev_alloc;
			}
			// update first free block?
			if (m_first_free_block == currBlock) {
				m_first_free_block = currBlock->next_alloc;
			}

			// delete pointers
			currBlock->next_alloc = nullptr;
			currBlock->prev_alloc = nullptr;
		}

		currBlock->next_alloc = nullptr;
		currBlock->prev_alloc = alloc_region_t::p_last_alloc;
		if (alloc_region_t::p_last_alloc != nullptr)
			alloc_region_t::p_last_alloc->next_alloc = currBlock;
		alloc_region_t::p_last_alloc = currBlock;
		t_tracking::register_allocation(currBlock, i_bytes, "no-desc", __FILE__, __LINE__);
		alloc_region_t::p_used_bytes += currBlock->frame_size;

		p_alloc_count++;
		return dataAddr;
	}
	// nothing found, cannot allocate anything
	return nullptr;
}

template <class t_tracking>
voidptr freelist_scheme<t_tracking>::reallocate(voidptr i_data, const size i_newBytes)
{
	floral::lock_guard memGuard(m_alloc_mutex);
	voidptr newAllocation = allocate(i_newBytes);

	if (newAllocation != nullptr) {
		alloc_header_t* releaseBlock = (alloc_header_t*)((p8)i_data - sizeof(alloc_header_t));
		size dataSizeBytes = releaseBlock->frame_size - sizeof(alloc_header_t) - HL_ALIGNMENT;

		// memcpy
		memcpy(newAllocation, i_data, dataSizeBytes);

		// now we can free the old data
		// FIXME: this will trigger the recursive mutex mechanism, it's BAD. Please google.
		free(i_data);

		return newAllocation;
	}
	return nullptr;
}

// free a block, update the free list
template <class t_tracking>
void freelist_scheme<t_tracking>::free_block(alloc_header_t* i_block, alloc_header_t* i_prevFree, alloc_header_t* i_nextFree)
{
	// erase its content
	p8 pData = (p8)i_block + sizeof(alloc_header_t);
	//memset(pData, 0, i_block->GetDataCapacity());
#if defined(ZERO_OUT_MEMORY)
	memset(pData, 0, i_block->frame_size - HL_ALIGNMENT - sizeof(alloc_header_t));
#endif
	i_block->next_alloc = nullptr;
	i_block->prev_alloc = nullptr;
	//i_block->TrackingInfo = nullptr;

	// adjust pointers
	if (i_prevFree) {
		i_block->prev_alloc = i_prevFree;
		i_prevFree->next_alloc = i_block;
	}
	if (i_nextFree) {
		i_block->next_alloc = i_nextFree;
		i_nextFree->prev_alloc = i_block;
	}
}

// check if 2 blocks can be joined
template <class t_tracking>
const bool freelist_scheme<t_tracking>::can_join(alloc_header_t* i_leftBlock, alloc_header_t* i_rightBlock)
{
	aptr leftEnd = (aptr)i_leftBlock - i_leftBlock->adjustment + i_leftBlock->frame_size;
	aptr rightStart = (aptr)i_rightBlock - i_rightBlock->adjustment;
	return (leftEnd == rightStart);
}

// join 2 *free* blocks together
template <class t_tracking>
const bool freelist_scheme<t_tracking>::join_blocks(alloc_header_t* i_leftBlock, alloc_header_t* i_rightBlock)
{
	if (!i_leftBlock || !i_rightBlock)
		return false;

	if (can_join(i_leftBlock, i_rightBlock)) {
		// adjust left block's pointers
		i_leftBlock->next_alloc = i_rightBlock->next_alloc;

		// update frame_size of left block
		i_leftBlock->frame_size += i_rightBlock->frame_size;

		if (i_rightBlock->next_alloc)
			i_rightBlock->next_alloc->prev_alloc = i_leftBlock;

		// erase right block header as we don't need it anymore
#if defined(ZERO_OUT_MEMORY)
		memset(i_rightBlock, 0, sizeof(alloc_header_t));
#endif
		return true;
	}
	else return false;
}

template <class t_tracking>
void freelist_scheme<t_tracking>::free(voidptr i_data)
{
	floral::lock_guard memGuard(m_alloc_mutex);
	alloc_header_t* releaseBlock = (alloc_header_t*)((p8)i_data - sizeof(alloc_header_t));
	alloc_region_t::p_used_bytes -= releaseBlock->frame_size;
	t_tracking::unregister_allocation(releaseBlock);
	p_free_count++;

	if (releaseBlock->next_alloc)
		releaseBlock->next_alloc->prev_alloc = releaseBlock->prev_alloc;
	if (releaseBlock->prev_alloc)
		releaseBlock->prev_alloc->next_alloc = releaseBlock->next_alloc;
	if (releaseBlock == alloc_region_t::p_last_alloc) {
		alloc_region_t::p_last_alloc = releaseBlock->prev_alloc;
	}

	// search for nearest-after free block
	alloc_header_t* nextFree = m_first_free_block;
	while (nextFree &&
		((aptr)nextFree <= (aptr)releaseBlock)) {
		nextFree = nextFree->next_alloc;
	}
	alloc_header_t* prevFree = nextFree->prev_alloc;

	// free releaseBlock
	free_block(releaseBlock, prevFree, nextFree);

	// update first free block
	if ((aptr)releaseBlock < (aptr)m_first_free_block) {
		m_first_free_block = releaseBlock;
	}
	// join blocks if possible
	if (join_blocks(prevFree, releaseBlock))
		join_blocks(prevFree, nextFree);
	else join_blocks(releaseBlock, nextFree);
}

template <class t_tracking>
void freelist_scheme<t_tracking>::free_all()
{
	floral::lock_guard memGuard(m_alloc_mutex);
#if defined(ZERO_OUT_MEMORY)
	memset(alloc_region_t::p_base_address, 0, alloc_region_t::p_size_in_bytes);
#endif

	m_first_free_block = (alloc_header_t*)alloc_region_t::p_base_address;
	m_first_free_block->frame_size = alloc_region_t::p_size_in_bytes;
	m_first_free_block->adjustment = 0;
	m_first_free_block->next_alloc = nullptr;
	m_first_free_block->prev_alloc = nullptr;
}
}
