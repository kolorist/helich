// declaration header
#include "MemoryManager.h"

// self-provided headers
#include "MemoryMap.h"
#include "StackWalker.h"

// 3rd-party headers
#if defined(_WIN32)
#include <Windows.h>
#include <iostream>
#endif

namespace helich {

	extern StackWalker							g_Walker;

MemoryManager::MemoryManager()
	: m_BaseAddress(nullptr)
{

}

MemoryManager::~MemoryManager()
{

}

const voidptr MemoryManager::AllocateGlobalMemory(voidptr baseAddress, const u32 sizeInBytes)
{
#if defined(_WIN32)
	m_BaseAddress = (voidptr)VirtualAlloc((LPVOID)baseAddress,
		sizeInBytes,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE);
#endif
	g_Walker.LoadModules();
	return m_BaseAddress;
}

}
