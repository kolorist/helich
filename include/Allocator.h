#ifndef __HL_ALLOCATOR_H__
#define __HL_ALLOCATOR_H__

#include "TrackingPolicies.h"

// 3rd-party headers
#include <stdaliases.h>
#include <new>

namespace helich {
	// note: template template parameter is the best example for indicating the different
	// between 'typename' and 'class'
	// we must use: template<typename> class Foo
	// instead of: template<typename> typename Foo
	// however, since C++17 (and VS2015) we can use them interchangeable in this case
	template <template<typename> class t_alloc_scheme, class t_tracking_policy = default_tracking_policy>
	class allocator :
		public t_alloc_scheme<t_tracking_policy>
	{
    public:
        typedef t_alloc_scheme<t_tracking_policy>         alloc_scheme_t;

	public:
		allocator();
		~allocator();

		template <class t_object_type>
		static const size get_real_data_size() {
			return t_alloc_scheme<t_tracking_policy>::get_real_data_size(sizeof(t_object_type));
		}

		static const size get_real_data_size(const size i_rawDataSize) {
			return t_alloc_scheme<t_tracking_policy>::get_real_data_size(i_rawDataSize);
		}

		template <class t_object_type>
		static const size get_closure_size(const size i_dataSize) {
			return (i_dataSize + sizeof(t_object_type) + HL_ALIGNMENT);
		}

		template <class t_object_type, class t_closure_allocator, class ... t_params>
		t_object_type* allocate_closure(const size i_bytes, t_params... i_params) {
			voidptr addr = t_alloc_scheme<t_tracking_policy>::allocate(i_bytes + sizeof(t_object_type) + HL_ALIGNMENT);
			t_object_type* obj = new (addr) t_object_type(i_params...);
			voidptr baseClsAddress = align_address((s8*)addr + sizeof(t_object_type));
			((t_closure_allocator*)obj)->map_tp(baseClsAddress, i_bytes, "");
			return obj;
		}

		template <class t_closure_allocator>
		t_closure_allocator* allocate_arena(const size i_bytes) {
			voidptr addr = t_alloc_scheme<t_tracking_policy>::allocate(i_bytes + sizeof(t_closure_allocator) + HL_ALIGNMENT);
			t_closure_allocator* cls = new (addr) t_closure_allocator();
			voidptr baseClsAddress = align_address((s8*)addr + sizeof(t_closure_allocator));
			cls->map_to(baseClsAddress, i_bytes, "arena");
			return cls;
		}

		voidptr allocate(const size i_bytes) {
			return t_alloc_scheme<t_tracking_policy>::allocate(i_bytes);
		}

		template <class t_object_type, class ... t_params>
		t_object_type* allocate(t_params... i_params) {
			voidptr addr = t_alloc_scheme<t_tracking_policy>::allocate(sizeof(t_object_type));
			return new (addr) t_object_type(i_params...);
		}

		template <class t_object_type>
		t_object_type* allocate_array(const u32 i_elemCount) {
			voidptr addr = t_alloc_scheme<t_tracking_policy>::allocate(sizeof(t_object_type) * i_elemCount);
			return new (addr) t_object_type[i_elemCount];
		}
	};

	//////////////////////////////////////////////////////////////////////////
	template <template<size, typename> class t_alloc_scheme, size t_elem_size, class t_tracking_policy = default_tracking_policy>
	class fixed_allocator :
		public t_alloc_scheme<t_elem_size, t_tracking_policy>
	{
    public:
        typedef t_alloc_scheme<t_elem_size, t_tracking_policy>  alloc_scheme_t;

	public:
		fixed_allocator()
		{}
		~fixed_allocator()
		{}

		template <class t_object_type, class ... t_params>
		t_object_type* allocate(t_params... i_params) {
			voidptr addr = t_alloc_scheme<t_elem_size, t_tracking_policy>::allocate();
			return new (addr) t_object_type(i_params...);
		}
	};
}

#include "Allocator.hpp"

#endif // __HL_ALLOCATOR_H__
