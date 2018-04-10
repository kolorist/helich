#ifndef __HL_TRACKING_POLICIES_H__
#define __HL_TRACKING_POLICIES_H__

// self-provided headers
#include "AllocHeaders.h"
#include "Allocator.h"
#include "AllocSchemes.h"

// 3rd-party headers
#include <stdaliases.h>

namespace helich {

	struct debug_entry {
		voidptr									address;
		size									size_in_bytes;
		c8										description[128];
		c8										stack_trace[2048];
	};

	class default_tracking_policy {
	public:
		typedef tracked_alloc_header			alloc_header_t;

	public:
		static void								register_allocation(voidptr i_dataAddr, const size i_bytes, const_cstr i_desc, const_cstr i_file, const u32 i_line);
		static void								unregister_allocation(voidptr i_ptr);

	private:
		static u32								m_num_alloc;
	};

	// This policy will be used mostly by 'release' build

	class no_tracking_policy {
	public:
		typedef untracked_alloc_header			alloc_header_t;

	public:
		static void								register_allocation(voidptr i_dataAddr, const size i_bytes, const_cstr i_desc, const_cstr i_file, const u32 i_line)	{}
		static void								unregister_allocation(voidptr i_ptr)																			{}
	};
}

#endif // __HL_TRACKING_POLICIES_H__
