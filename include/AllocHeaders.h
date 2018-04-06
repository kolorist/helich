#ifndef __HL_ALLOC_HEADERS_H__
#define __HL_ALLOC_HEADERS_H__

// 3rd-party headers
#include <stdaliases.h>

namespace helich {

	/*
	 * types of allocation header:
	 *	> VariableSize
	 *		>> Tracked
	 *		>> Untracked
	 *	> FixedSize
	 *		>> Tracked
	 *		>> Untracked
	 */

	template <class t_tracking_header>
	struct fixed_size_alloc_header : t_tracking_header {
		fixed_size_alloc_header*				next_alloc;
		fixed_size_alloc_header*				prev_alloc;
		size									frame_size;
		size									adjustment;			// cannot use u8 for arithmetic operations,
																	// because we will have to downcast from u64 / u32 -> u8
	};

	template <class t_tracking_header>
	struct variable_size_alloc_header : t_tracking_header {
		variable_size_alloc_header*				next_alloc;
		variable_size_alloc_header*				prev_alloc;
		size									frame_size;
		size									adjustment;
	};

	struct debug_entry;
	struct tracked_alloc_header {
		debug_entry*							debug_info;
	};

	struct untracked_alloc_header {
	};
}

#endif // __HL_ALLOC_HEADERS_H__
