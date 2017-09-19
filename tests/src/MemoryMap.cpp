#include "MemoryMap.h"

MemoryManager									g_MemoryManager;

Allocator<StackScheme, NoTrackingPolicy>		g_TestAllocator;
Allocator<FreelistScheme, NoTrackingPolicy>		g_FreelistAllocator;
FixedAllocator<PoolScheme, 27, DefaultTrackingPolicy>	g_PoolAllocator;