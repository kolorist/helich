#pragma once

// self-provided headers
#include "macros.h"
#include "detail/alloc_region.h"
#include "alloc_headers.h"

// 3rd-party headers
#include <floral.h>

namespace helich {

template <class t_tracking>
class stack_scheme : 
	private detail::alloc_region<variable_size_alloc_header<typename t_tracking::alloc_header_t> > 
{
public:
	typedef typename t_tracking::alloc_header_t         	tracking_header_t;
	typedef variable_size_alloc_header<tracking_header_t>	alloc_header_t;
	typedef detail::alloc_region<alloc_header_t>        	alloc_region_t;

public:
	stack_scheme();
	
	void									map_to(voidptr i_baseAddress, const size i_sizeInBytes, const_cstr i_name);
	voidptr									allocate(const size i_bytes, const_cstr i_desc = nullptr);
	// NOTE 1: 'reallocate' function of 'stack_scheme' will not free the old data, it shall become wasted
	// why? because it's stack scheme, we are not gonna check if the reallocating region is the top one in the stack
	// though it's possible to do so
	// NOTE 2: and please, only use reallocate() in transient allocators
	voidptr									reallocate(voidptr i_data, const size i_newBytes);
	void									free(voidptr i_data);

	void									free_all();

	//////////////////////////////////////////////////////////////////////////
	static const size						get_real_data_size(const size i_dataSize)				{ return (i_dataSize + HL_ALIGNMENT + sizeof(alloc_header_t)); }

	// NOTE: the destructor of policy class should be protected to prevent any attempts to delete
	// the host class by using pointers to its derived class (which is the policy class here)
	// When delete the host class by its original pointers, the destructors are called correctly in
	// both derived class and base class
protected:
	~stack_scheme();
	
private:
	p8										m_current_marker;
	
public:
	const p8									get_base_address() const 						{ return alloc_region_t::p_base_address; }
	const size									get_size_in_bytes() const						{ return alloc_region_t::p_size_in_bytes; }
	const size									get_used_bytes() const							{ return alloc_region_t::p_used_bytes; }
	const size									get_remain_bytes() const						{ return alloc_region_t::p_size_in_bytes - alloc_region_t::p_used_bytes - HL_ALIGNMENT - sizeof(alloc_header_t); }
};

//////////////////////////////////////////////////////////////////////////

template <size t_elem_size, class t_tracking>
class pool_scheme : 
	private detail::alloc_region<fixed_size_alloc_header<typename t_tracking::alloc_header_t> >
{
public:
	typedef typename t_tracking::alloc_header_t				tracking_header_t;
	typedef fixed_size_alloc_header<tracking_header_t>		alloc_header_t;
	typedef detail::alloc_region<alloc_header_t>			alloc_region_t;

public:
	pool_scheme();

	void									map_to(voidptr i_baseAddress, const size i_sizeInBytes, const_cstr i_name);
	voidptr									allocate(const_cstr i_desc = nullptr);
	void									free(voidptr i_data);

	void									free_all();

protected:
	~pool_scheme();

private:
	alloc_header_t*							m_next_free_slot;
	size									m_element_size;
	u32										m_element_count;
	
public:
	const p8									get_base_address() const 						{ return alloc_region_t::p_base_address; }
	const size									get_size_in_bytes() const						{ return alloc_region_t::p_size_in_bytes; }
	const size									get_used_bytes() const							{ return alloc_region_t::p_used_bytes; }
	const size									get_remain_bytes() const						{ return 0; }
};

//////////////////////////////////////////////////////////////////////////

template <class t_tracking>
class freelist_scheme : 
	private detail::alloc_region<variable_size_alloc_header<typename t_tracking::alloc_header_t> >
{
public:
	typedef typename t_tracking::alloc_header_t				tracking_header_t;
	typedef variable_size_alloc_header<tracking_header_t>	alloc_header_t;
	typedef detail::alloc_region<alloc_header_t>			alloc_region_t;

public:
	freelist_scheme();

	void									map_to(voidptr i_baseAddress, const size i_sizeInBytes, const_cstr i_name);
	voidptr									allocate(const size i_bytes, const_cstr i_desc = nullptr);
	voidptr									reallocate(voidptr i_data, const size i_newBytes);
	void									free(voidptr i_data);

	void									free_all();

	//////////////////////////////////////////////////////////////////////////
	static const size						get_real_data_size(const size i_dataSize)				{ return (i_dataSize + HL_ALIGNMENT + sizeof(alloc_header_t)); }

private:
	static inline const bool				can_fit(alloc_header_t* i_header, const size i_bytes);
	static inline const bool				can_create_new_block(alloc_header_t* i_header, const size i_bytes, const size i_minFrameSize);

	static void								free_block(alloc_header_t* i_block, alloc_header_t* i_prevFree, alloc_header_t* i_nextFree);
	static const bool						join_blocks(alloc_header_t* i_leftBlock, alloc_header_t* i_rightBlock);
	static const bool						can_join(alloc_header_t* i_leftBlock, alloc_header_t* i_rightBlock);

protected:
	~freelist_scheme();

private:
	const size								k_min_frame_size;
	alloc_header_t*							m_first_free_block;

public:
	const p8									get_base_address() const 						{ return alloc_region_t::p_base_address; }
	const size									get_size_in_bytes() const						{ return alloc_region_t::p_size_in_bytes; }
	const size									get_used_bytes() const							{ return alloc_region_t::p_used_bytes; }
	const size									get_remain_bytes() const						{ return 0; }

	u32										p_alloc_count;
	u32										p_free_count;
};

}

#include "alloc_schemes.hpp"
