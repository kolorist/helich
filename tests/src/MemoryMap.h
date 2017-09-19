#ifndef __HL_UTEST_MEMORY_MAP_H__
#define __HL_UTEST_MEMORY_MAP_H__

#include <helich.h>

using namespace helich;

extern MemoryManager							g_MemoryManager;

extern Allocator<StackScheme, NoTrackingPolicy>	g_TestAllocator;
extern Allocator<FreelistScheme, NoTrackingPolicy> g_FreelistAllocator;
extern FixedAllocator<PoolScheme, 27, DefaultTrackingPolicy> g_PoolAllocator;

#endif // __HL_UTEST_MEMORY_MAP_H__