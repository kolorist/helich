#ifndef __HL_ALLOC_SCHEMES_H__
#define __HL_ALLOC_SCHEMES_H__

// self-provided headers
#include "macros.h"
#include "detail/AllocRegion.h"
#include "AllocHeaders.h"

// 3rd-party headers
#include <stdaliases.h>

namespace helich {

	template <class PTracking>
	class StackScheme : 
		private detail::AllocRegion<VariableSizeAllocHeader<typename PTracking::AllocHeaderType> > 
	{
	private:
		typedef VariableSizeAllocHeader<typename PTracking::AllocHeaderType> AllocHeaderType;

	public:
		StackScheme();
		
		void									MapTo(voidptr baseAddress, const u32 sizeInBytes, const_cstr name);
		voidptr									Allocate(const u32 nBytes);
		void									Free(voidptr pData);

		void									FreeAll();

		//////////////////////////////////////////////////////////////////////////
		static const u32						GetRealDataSize(const u32 dataSize)				{ return (dataSize + HL_ALIGNMENT + sizeof(AllocHeaderType)); }

		// NOTE: the destructor of policy class should be protected to prevent any attempts to delete
		// the host class by using pointers to its derived class (which is the policy class here)
		// When delete the host class by its original pointers, the destructors are called correctly in
		// both derived class and base class
	protected:
		~StackScheme();
		
	private:
		s8*										m_CurrentMarker;
	};

	//////////////////////////////////////////////////////////////////////////

	template <u32 UElemSize, class PTracking>
	class PoolScheme : 
		private detail::AllocRegion<FixedSizeAllocHeader<typename PTracking::AllocHeaderType> >
	{
	private:
		typedef FixedSizeAllocHeader<typename PTracking::AllocHeaderType> AllocHeaderType;

	public:
		PoolScheme();

		void									MapTo(voidptr baseAddress, const u32 sizeInBytes, const_cstr name);
		voidptr									Allocate();
		void									Free(voidptr pData);

		void									FreeAll();

	protected:
		~PoolScheme();

	private:
		AllocHeaderType*						m_LastAlloc;
		AllocHeaderType*						m_NextFreeSlot;
		u32										m_ElementSize;
		u32										m_ElementCount;
	};

	//////////////////////////////////////////////////////////////////////////

	template <class PTracking>
	class FreelistScheme : 
		private detail::AllocRegion<VariableSizeAllocHeader<typename PTracking::AllocHeaderType> >
	{
	private: 
		typedef VariableSizeAllocHeader<typename PTracking::AllocHeaderType> AllocHeaderType;

	public:
		FreelistScheme();

		void									MapTo(voidptr baseAddress, const u32 sizeInBytes, const_cstr name);
		voidptr									Allocate(const u32 nBytes);
		void									Free(voidptr pData);

		void									FreeAll();

		//////////////////////////////////////////////////////////////////////////
		static const u32						GetRealDataSize(const u32 dataSize)				{ return (dataSize + HL_ALIGNMENT + sizeof(AllocHeaderType)); }

	private:
		static inline const bool				CanFit(AllocHeaderType* header, const u32 nBytes);
		static inline const bool				CanCreateNewBlock(AllocHeaderType* header, const u32 nBytes, const u32 minFrameSize);

		static void								FreeBlock(AllocHeaderType* block, AllocHeaderType* prevFree, AllocHeaderType* nextFree);
		static const bool						JoinBlocks(AllocHeaderType* leftBlock, AllocHeaderType* rightBlock);
		static const bool						CanJoin(AllocHeaderType* leftBlock, AllocHeaderType* rightBlock);

		// for unit testing only
	public:
		const u32								UT_GetAllocationCountFromLL() const;
		const u32								UT_GetFreeBlockCountFromLL() const;
		AllocHeaderType*						UT_GetLastAlloc() const					{ return m_LastAlloc; }
		AllocHeaderType*						UT_GetFirstFree() const					{ return m_FirstFreeBlock; }

	protected:
		~FreelistScheme();

	private:
		AllocHeaderType*						m_FirstFreeBlock;
		AllocHeaderType*						m_LastAlloc;
		const u32								k_MinFrameSize;
	};

}

#include "AllocSchemes.hpp"

#endif // __HL_ALLOC_SCHEMES_H__