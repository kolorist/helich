#ifndef __HL_ALLOCATION_TRACKER_H__
#define __HL_ALLOCATION_TRACKER_H__

#include "macros.h"

#include "PoolAllocator.h"

#include <stdaliases.h>

namespace helich {

class allocation_tracker {
public:
	static const u32							sk_header_pool_size = 512;

public:
	allocation_tracker();
	~allocation_tracker();

	void										initialize();
	void										register_allocation(voidptr i_ptr, const size i_bytes, const_cstr i_desc, const_cstr i_file, const u32 i_line);
	void										unregister_allocation(voidptr i_ptr);
	void										do_sanity_check();
	const u32									get_allocation_count() const		{ return m_num_alloc; }
	const u32									get_free_count() const				{ return m_num_free; }

private:
	u32											m_num_alloc;
	u32											m_num_free;
};

extern allocation_tracker						g_allocation_tracker;

}

#endif // __HL_ALLOCATION_TRACKER_H__
