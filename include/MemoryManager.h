#ifndef __HELICH_MEMORY_MANAGER_H__
#define __HELICH_MEMORY_MANAGER_H__

#include <floral.h>

#include "macros.h"
#include "MemoryMap.h"
#include "Allocator.h"
#include "AllocSchemes.h"
#include "TrackingPolicies.h"
#include "detail/AllocRegion.h"

namespace helich {

	// this function need to be called by users before using helich
	extern void init_memory_system();

	extern fixed_allocator<pool_scheme, sizeof(debug_entry), no_tracking_policy> g_tracking_allocator;
#define MEMORY_TRACKING_SIZE					SIZE_MB(32)
#define MAX_MEM_REGIONS							32

class memory_manager {
public:
	memory_manager();
	~memory_manager();

	const voidptr								allocate_global_memory(voidptr i_baseAddress, const size i_sizeInBytes);

	template <class ... t_allocator_regions>
	const void initialize(t_allocator_regions ... i_regions) {
		if (!m_base_address) {
			size totalSize = internal_compute_mem(i_regions...);

			// this call will update m_base_address
			allocate_global_memory(nullptr, totalSize);
			p_mem_regions_count = 0;

			internal_init(m_base_address, 
				i_regions...);
		}
	}

private:
	template <class t_allocator_type>
	const size internal_compute_mem(memory_region<t_allocator_type> i_al)
	{
		return i_al.size_in_bytes + MEMORY_TRACKING_SIZE;
	}

	template <class t_allocator_type_head, class ... t_allocator_type_rests>
	const size internal_compute_mem(memory_region<t_allocator_type_head> i_headAl,
		memory_region<t_allocator_type_rests> ... i_restAl)
	{
		return (i_headAl.size_in_bytes + internal_compute_mem(i_restAl...));
	}

	template <class t_allocator_type>
	const bool InternalInitTracking(voidptr i_baseAddress, 
		memory_region<t_allocator_type> i_al) 
	{
        typedef typename t_allocator_type::alloc_scheme_t scheme_t;
        typedef typename scheme_t::alloc_region_t region_t;

		((t_allocator_type*)(i_al.allocator_ptr))->map_to(i_baseAddress, i_al.size_in_bytes, i_al.name);

		strcpy(p_mem_regions[p_mem_regions_count].name, "helich/tracking");
		p_mem_regions[p_mem_regions_count].size_in_bytes = MEMORY_TRACKING_SIZE;
		p_mem_regions[p_mem_regions_count].base_address = i_baseAddress;
        p_mem_regions[p_mem_regions_count].dbg_info_extractor = (dbginfo_extractor_func_t)&alloc_region_dbginfo_extractor<region_t>::extract_info;
		p_mem_regions[p_mem_regions_count].allocator_ptr = (voidptr)i_al.allocator_ptr;
		p_total_mem_in_bytes += MEMORY_TRACKING_SIZE;
		p_mem_regions_count++;
		return true;
	}

	// end of recursion
	template <class t_allocator_type>
	const bool internal_init(voidptr i_baseAddress,
		memory_region<t_allocator_type> i_al)
	{
        typedef typename t_allocator_type::alloc_scheme_t scheme_t;
        typedef typename scheme_t::alloc_region_t region_t;

		((t_allocator_type*)(i_al.allocator_ptr))->map_to(i_baseAddress, i_al.size_in_bytes, i_al.name);

		strcpy(p_mem_regions[p_mem_regions_count].name, i_al.name);
		p_mem_regions[p_mem_regions_count].size_in_bytes = i_al.size_in_bytes;
		p_mem_regions[p_mem_regions_count].base_address = i_baseAddress;
        p_mem_regions[p_mem_regions_count].dbg_info_extractor = (dbginfo_extractor_func_t)&alloc_region_dbginfo_extractor<region_t>::extract_info;
		p_mem_regions[p_mem_regions_count].allocator_ptr = (voidptr)i_al.allocator_ptr;
		p_total_mem_in_bytes += i_al.size_in_bytes;
		p_mem_regions_count++;

		// last one, tracking debug info pool
		s8* nextBase = (s8*)i_baseAddress + i_al.size_in_bytes;
		InternalInitTracking(nextBase,
			memory_region<fixed_allocator<pool_scheme, sizeof(debug_entry), no_tracking_policy>> { "helich/tracking", MEMORY_TRACKING_SIZE, &g_tracking_allocator });
		return true;
	}

	// compile-time recursive initialization
	template <class t_allocator_type_head, class ... t_allocator_type_rests>
	const bool internal_init(voidptr i_baseAddress,
		memory_region<t_allocator_type_head> i_headAl,
		memory_region<t_allocator_type_rests> ... i_restAl)
	{
        typedef typename t_allocator_type_head::alloc_scheme_t scheme_t;
        typedef typename scheme_t::alloc_region_t region_t;

		// init here
		((t_allocator_type_head*)(i_headAl.allocator_ptr))->map_to(i_baseAddress, i_headAl.size_in_bytes, i_headAl.name);
		s8* nextBase = (s8*)i_baseAddress + i_headAl.size_in_bytes;
		
		strcpy(p_mem_regions[p_mem_regions_count].name, i_headAl.name);
		p_mem_regions[p_mem_regions_count].size_in_bytes = i_headAl.size_in_bytes;
		p_mem_regions[p_mem_regions_count].base_address = i_baseAddress;
        p_mem_regions[p_mem_regions_count].dbg_info_extractor = (dbginfo_extractor_func_t)&alloc_region_dbginfo_extractor<region_t>::extract_info;
		p_mem_regions[p_mem_regions_count].allocator_ptr = (voidptr)i_headAl.allocator_ptr;
		p_total_mem_in_bytes += i_headAl.size_in_bytes;
		p_mem_regions_count++;

		// recursion
		return internal_init(nextBase, i_restAl...);
	}
	
private:
	voidptr                                 	m_base_address;

public:
	memory_region_info							p_mem_regions[MAX_MEM_REGIONS];
	u32											p_mem_regions_count;
	size										p_total_mem_in_bytes;
};

}

#endif // __HELICH_MEMORY_MANAGER_H__
