// declaration header
#include "MemoryManager.h"

// self-provided headers
#include "MemoryMap.h"

// 3rd-party headers
#if defined(_WIN32)
#include <Windows.h>
#include <iostream>
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
#if defined(_WIN32)
	m_base_address = (voidptr)VirtualAlloc((LPVOID)i_baseAddress,
		i_sizeInBytes,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE);
#endif
	return m_base_address;
}

}
