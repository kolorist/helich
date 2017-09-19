#include "MemoryMap.h"

#include <gtest/gtest.h>
#include <helich.h>

// fixture test
class Stack_Test : public testing::Test {
protected:
	static void InitMemory() {
		g_MemoryManager.Initialize(
			MemoryRegion<Allocator<StackScheme, NoTrackingPolicy>> { "test", SIZE_KB(32), &g_TestAllocator },
			MemoryRegion<Allocator<FreelistScheme, NoTrackingPolicy>> { "freelist", SIZE_KB(32), &g_FreelistAllocator },
			MemoryRegion<FixedAllocator<PoolScheme, 27, DefaultTrackingPolicy>> { "pool", SIZE_KB(32), &g_PoolAllocator }
		);
	}

	virtual void SetUp() {
		InitMemory();
	}
};

typedef Allocator<StackScheme, NoTrackingPolicy> StackMemoryClosure;

class TestClosure : public StackMemoryClosure {
public:
	TestClosure()
		: m_C('c')
		, m_A(0x12345678)
		, m_B('b')
		, m_D(22.22f)
	{}

	void TestAlloc() {
		int* a = Allocate<int>();
		*a = 0x22222222;
		char* b1 = Allocate<char>();
		*b1 = 'b';
		int* a1 = Allocate<int>();
		*a1 = 0x33333333;
	}

private:
	char m_C;
	int m_A;
	char m_B;
	float m_D;
};

TEST_F(Stack_Test, Pool_Allocation)
{
	int* a1 = g_PoolAllocator.Allocate<int>();
	*a1 = 0x11111111;
	int* a2 = g_PoolAllocator.Allocate<int>();
	*a2 = 0x22222222;
	int* a3 = g_PoolAllocator.Allocate<int>();
	*a3 = 0x33333333;

	g_PoolAllocator.Free(a2);

	int* a4 = g_PoolAllocator.Allocate<int>();
	*a4 = 0x44444444;
}

TEST_F(Stack_Test, Primitives_Allocate_And_Free)
{
	for (int tt = 0; tt < 512; tt++) {
		int* a = g_TestAllocator.Allocate<int>();
		*a = 10;
		EXPECT_EQ(*a, 10);

		int* aa = g_TestAllocator.Allocate<int>(11);
		EXPECT_EQ(*aa, 11);

		float *b = g_TestAllocator.Allocate<float>();
		*b = 11.1f;
		EXPECT_EQ(*b, 11.1f);

		float *bb = g_TestAllocator.Allocate<float>(22.2f);
		EXPECT_EQ(*bb, 22.2f);

		// array allocation
		int* arr1 = g_TestAllocator.AllocateArray<int>(10);
		for (unsigned int i = 0; i < 10; i++) {
			arr1[i] = i;
		}
		EXPECT_EQ(arr1[5], 5);
		EXPECT_EQ(arr1[9], 9);

		g_TestAllocator.Free(arr1);

		//ASSERT_DEATH(g_TestAllocator.Free(b), "");

		g_TestAllocator.Free(bb);
		g_TestAllocator.Free(b);
		g_TestAllocator.Free(aa);
		g_TestAllocator.Free(a);

		TestClosure *tcls = g_TestAllocator.AllocateClosure<TestClosure, StackMemoryClosure>(91);
		tcls->TestAlloc();
		TestClosure *tcls2 = g_TestAllocator.AllocateClosure<TestClosure, StackMemoryClosure>(91);
		tcls2->TestAlloc();

		g_TestAllocator.Free(tcls2);
		g_TestAllocator.Free(tcls);
	}
}

TEST_F(Stack_Test, Freelist_Alloc)
{
	typedef VariableSizeAllocHeader<NoTrackingPolicy::AllocHeaderType> HeaderType;
	HeaderType* lastAlloc = nullptr;
	HeaderType* firstFree = nullptr;

	//////////////////////////////////////////////////////////////////////////
	int* a = g_FreelistAllocator.Allocate<int>();
	*a = 0x11111111;
	EXPECT_EQ(*a, 0x11111111);

	lastAlloc = g_FreelistAllocator.UT_GetLastAlloc();
	EXPECT_EQ(lastAlloc->FrameSize, sizeof(HeaderType) + sizeof(int) + HL_ALIGNMENT);

	firstFree = g_FreelistAllocator.UT_GetFirstFree();
	//////////////////////////////////////////////////////////////////////////
	char* a1 = g_FreelistAllocator.Allocate<char>('e');
	EXPECT_EQ(*a1, 'e');

	char* a2 = g_FreelistAllocator.Allocate<char>('f');
	EXPECT_EQ(*a2, 'f');

	char* a3 = g_FreelistAllocator.Allocate<char>('g');
	EXPECT_EQ(*a3, 'g');

	char* a4 = g_FreelistAllocator.Allocate<char>('h');
	EXPECT_EQ(*a4, 'h');

	EXPECT_EQ(g_FreelistAllocator.UT_GetAllocationCountFromLL(), 5);
	EXPECT_EQ(g_FreelistAllocator.UT_GetFreeBlockCountFromLL(), 1);

	//////////////////////////////////////////////////////////////////////////
	int* b = g_FreelistAllocator.Allocate<int>(0x22222222);
	EXPECT_EQ(*b, 0x22222222);
	int* c = g_FreelistAllocator.Allocate<int>();
	*c = 0x33333333;
	EXPECT_EQ(*c, 0x33333333);
	int* d = g_FreelistAllocator.Allocate<int>(0x44444444);
	EXPECT_EQ(*d, 0x44444444);


	g_FreelistAllocator.Free(c);


	g_FreelistAllocator.Free(d);
	//g_FreelistAllocator.Free(a);
	g_FreelistAllocator.Free(b);
}