How to use 'helich' memory manager
---------------------------------------------------------------------------------------------------------
1- Create a header-source file pairs
	MemorySystem.h
		#include <helich.h>
		using namespace helich;
		extern MemoryManager							g_MemoryManager;
		extern Allocator<StackScheme, NoTrackingPolicy>	g_TestAllocator;

	MemorySystem.cpp
		#include "MemorySystem.h"
		MemoryManager									g_MemoryManager;
		Allocator<StackScheme, NoTrackingPolicy>		g_TestAllocator;

2- Init the memory manager
	g_MemoryManager.Initialize(
		MemoryRegion<Allocator<StackScheme, NoTrackingPolicy>> { "test", SIZE_KB(32), &g_TestAllocator }
	);

3- Use it
	int* a = g_TestAllocator.Allocate<int>();
	*a = 10;
