#pragma once

#include "alloc_headers.h"
#include "alloc_schemes.h"

#include <floral/stdaliases.h>

namespace helich
{
// ----------------------------------------------------------------------------

struct debug_entry
{
	voidptr										address;
	size										size_in_bytes;
	c8											description[128];
	c8											stack_trace[2048];
};

class default_tracking_policy
{
public:
	typedef tracked_alloc_header				alloc_header_t;

public:
	static void									register_allocation(voidptr i_dataAddr, const size i_bytes, const_cstr i_desc, const_cstr i_file, const u32 i_line);
	static void									unregister_allocation(voidptr i_ptr);

private:
	static u32									m_num_alloc;
};

// This policy will be used mostly by 'release' build

class no_tracking_policy
{
public:
	typedef untracked_alloc_header				alloc_header_t;

public:
	static void									register_allocation(voidptr i_dataAddr, const size i_bytes, const_cstr i_desc, const_cstr i_file, const u32 i_line)	{}
	static void									unregister_allocation(voidptr i_ptr)																			{}
};

// ----------------------------------------------------------------------------
}
