#include "helich/MemoryManager.h"

#include "helich/MemoryMap.h"

#if defined(PLATFORM_WINDOWS)
#	include <Windows.h>
#	include <iostream>
#else
#endif

namespace helich {

memory_manager::memory_manager()
	: m_base_address(nullptr)
{

}

memory_manager::~memory_manager()
{

}

const voidptr memory_manager::allocate_global_memory(voidptr i_baseAddress, const size i_sizeInBytes)
{
#if defined(PLATFORM_WINDOWS)
	m_base_address = (voidptr)VirtualAlloc((LPVOID)i_baseAddress,
		i_sizeInBytes,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE);
#else
	m_base_address = (voidptr)malloc(i_sizeInBytes);
#endif
	return m_base_address;
}

}
