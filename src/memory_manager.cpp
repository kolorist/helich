#include "helich/memory_manager.h"

#include "helich/memory_map.h"

#if defined(FLORAL_PLATFORM_WINDOWS)
#	include <Windows.h>
#	include <iostream>
#else
#endif

namespace helich
{
// ----------------------------------------------------------------------------

memory_manager::memory_manager()
	: m_base_address(nullptr)
{

}

memory_manager::~memory_manager()
{

}

const voidptr memory_manager::allocate_global_memory(voidptr i_baseAddress, const size i_sizeInBytes)
{
#if defined(FLORAL_PLATFORM_WINDOWS)
	voidptr addr = (voidptr)VirtualAlloc((LPVOID)i_baseAddress,
		i_sizeInBytes,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE);
#else
	voidptr addr = (voidptr)malloc(i_sizeInBytes);
#endif
	return addr;
}

void memory_manager::free_global_memory(voidptr i_baseAddress, const size i_sizeInBytes)
{
#if defined(FLORAL_PLATFORM_WINDOWS)
	BOOL result = VirtualFree((LPVOID)i_baseAddress, 0, MEM_RELEASE);
	const DWORD error = GetLastError();
	FLORAL_ASSERT(result != 0);
#else
	free(i_baseAddress);
#endif
}

// ----------------------------------------------------------------------------
}
