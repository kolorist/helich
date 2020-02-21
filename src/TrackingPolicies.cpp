#include "helich/TrackingPolicies.h"

#include "helich/Allocator.h"

// 3rd-party headers
#include <cstring>

namespace helich {

	fixed_allocator<pool_scheme, sizeof(debug_entry), no_tracking_policy> g_tracking_allocator;

	//////////////////////////////////////////////////////////////////////////
	// Default Tracking Policy
	//////////////////////////////////////////////////////////////////////////

	u32 default_tracking_policy::m_num_alloc = 0;
	
	void default_tracking_policy::register_allocation(voidptr i_dataAddr, const size i_bytes, const_cstr i_desc, const_cstr i_file, const u32 i_line)
	{
		// get memory header
		alloc_header_t* memHeader = (alloc_header_t*)i_dataAddr;

		// allocate new TrackingEntry
		debug_entry* newEntry = g_tracking_allocator.allocate<debug_entry>();

		// populate allocation information
		strcpy(newEntry->description, i_desc);
		newEntry->size_in_bytes = i_bytes;
		newEntry->address = i_dataAddr;
		newEntry->stack_trace[0] = 0;
#if defined(PLATFORM_WINDOWS)
		floral::get_stack_trace(newEntry->stack_trace);
#elif defined(PLATFORM_POSIX)
		// TODO: add
#endif

		// update memory header info
		memHeader->debug_info = newEntry;

		m_num_alloc++;
	}

	void default_tracking_policy::unregister_allocation(voidptr i_ptr)
	{
		// get memory header
		alloc_header_t* memHeader = (alloc_header_t*)i_ptr;

		// and free tracking info
		g_tracking_allocator.free(memHeader->debug_info);

		memHeader->debug_info = nullptr;

		m_num_alloc--;
	}

}
