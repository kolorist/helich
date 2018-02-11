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
	template <template<typename> class TAllocScheme, class PTracking = DefaultTrackingPolicy>
	class Allocator : 
		public TAllocScheme<PTracking>
	{
    public:
        typedef typename TAllocScheme<typename PTracking>         AllocSchemeType;

	public:
		Allocator();
		~Allocator();

		template <class TObjectType>
		static const u32 GetRealDataSize() {
			return TAllocScheme<PTracking>::GetRealDataSize(sizeof(TObjectType));
		}

		static const u32 GetRealDataSize(const u32 rawDataSize) {
			return TAllocScheme<PTracking>::GetRealDataSize(rawDataSize);
		}

		template <class TObjectType>
		static const u32 GetClosureSize(const u32 dataSize) {
			return (dataSize + sizeof(TObjectType) + HL_ALIGNMENT);
		}

		template <class TObjectType, class TClosureAllocator, class ... TParams>
		TObjectType* AllocateClosure(const u32 nBytes, TParams... params) {
			voidptr addr = TAllocScheme<PTracking>::Allocate(nBytes + sizeof(TObjectType) + HL_ALIGNMENT);
			TObjectType* obj = new (addr) TObjectType(params...);
			voidptr baseClsAddress = alignAddress((s8*)addr + sizeof(TObjectType));
			((TClosureAllocator*)obj)->MapTo(baseClsAddress, nBytes, "");
			return obj;
		}

		template <class TClosureAllocator>
		TClosureAllocator* AllocateArena(const u32 nBytes) {
			voidptr addr = TAllocScheme<PTracking>::Allocate(nBytes + sizeof(TClosureAllocator) + HL_ALIGNMENT);
			TClosureAllocator* cls = new (addr) TClosureAllocator();
			voidptr baseClsAddress = alignAddress((s8*)addr + sizeof(TClosureAllocator));
			cls->MapTo(baseClsAddress, nBytes, "arena");
			return cls;
		}

		voidptr Allocate(const u32 nBytes) {
			return TAllocScheme<PTracking>::Allocate(nBytes);
		}

		template <class TObjectType, class ... TParams>
		TObjectType* Allocate(TParams... params) {
			voidptr addr = TAllocScheme<PTracking>::Allocate(sizeof(TObjectType));
			return new (addr) TObjectType(params...);
		}

		template <class TObjectType>
		TObjectType* AllocateArray(const u32 elemCount) {
			voidptr addr = TAllocScheme<PTracking>::Allocate(sizeof(TObjectType) * elemCount);
			return new (addr) TObjectType[elemCount];
		}
	};

	//////////////////////////////////////////////////////////////////////////
	template <template<u32, typename> class TAllocScheme, u32 UElemSize, class PTracking = DefaultTrackingPolicy>
	class FixedAllocator :
		public TAllocScheme<UElemSize, PTracking>
	{
    public:
        typedef typename TAllocScheme<UElemSize, typename PTracking>  AllocSchemeType;

	public:
		FixedAllocator()
		{}
		~FixedAllocator()
		{}

		template <class TObjectType, class ... TParams>
		TObjectType* Allocate(TParams... params) {
			voidptr addr = TAllocScheme<UElemSize, PTracking>::Allocate();
			return new (addr) TObjectType(params...);
		}
	};
}

#include "Allocator.hpp"

#endif // __HL_ALLOCATOR_H__
